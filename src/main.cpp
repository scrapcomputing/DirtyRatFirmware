//-*- C++ -*-
//
// Copyright (C) 2023 Scrap Computing
//
// This is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.
// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
// You should have received a copy of the GNU General Public License
// along with GCC; see the file LICENSE.  If not see
// <http://www.gnu.org/licenses/>.

#include "Isa.h"
#include "Mouse.h"
#include "Pico.h"
#include "bsp/board.h"
#include "tusb.h"
#include <chrono>
#include <config.h>
#include <hardware/watchdog.h>

// This can be set by cmake with -DIO_ADDR=
#ifndef IO_ADDR
#define IO_ADDR 0x2e8
#endif

static Mouse MouseState;
// This is true if there is mouse data available.
bool MouseDataReady = false;

static ISABus *ISA = nullptr;

/// A simple guard class that makes sure we call tuh_hid_receive_report()
class TuhHidReceiveReportGuard {
  uint8_t DevAddr;
  uint8_t Instance;

public:
  TuhHidReceiveReportGuard(uint8_t DevAddr, uint8_t Instance)
      : DevAddr(DevAddr), Instance(Instance) {}
  ~TuhHidReceiveReportGuard() {
    bool ReportOK = tuh_hid_receive_report(DevAddr, Instance);
    if (!ReportOK)
      std::cerr << "Error in receive report\n";
  }
};

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const *descr_report, uint16_t desc_len) {
  TuhHidReceiveReportGuard ReceiveReportGuard(dev_addr, instance);
  if (tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_MOUSE)
    std::cerr << "Mouse mount!\n";
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  if (tuh_hid_interface_protocol(dev_addr, instance))
    std::cerr << "Mouse umount\n";
}

/// Sends the mouse data to the ISA bus for the \p MousePacketIdx'th mouse
/// packet. This function busy loops until it receives an ISA IOR read and then
/// writes the correct mouse packet data to the ISA bus.
/// It also clears the IRQ within some time, or before returning.
static void sendMousePacket(ISABus &ISA, uint32_t MousePacketIdx) {
  ISA.setIRQ();
  // A counter to send out an interrupt. This is for testing.
  ISA.readBus();
  uint32_t LastIOR = ISA.getIOR();
  uint32_t ThisIOR;
  uint32_t LoopCnt = 0;
  static constexpr const uint32_t InfiniteLoopCnt = 10000;
  for (;; LastIOR = ThisIOR, LoopCnt++) {
    ISA.readBus();
    ThisIOR = ISA.getIOR();

    // Check for falling edge
    if (ThisIOR == 0u && ThisIOR != LastIOR) {
      uint32_t Addr = ISA.getAddr();
      bool IsRead = Addr == IO_ADDR;
      if (!IsRead)
        continue;
      // Clear IORDY early to be on the safe side.
      ISA.clearIORDY();
      ISA.clearIRQ();
      // Lower IOCHRDY as soon as possible to buy some time.
      switch (MousePacketIdx) {
      case 0:
        ISA.writeBusCycle(MouseState.getFirstByteMS());
        break;
      case 1:
        ISA.writeBusCycle(MouseState.getSecondByteMS());
        break;
      case 2:
        ISA.writeBusCycle(MouseState.getThirdByteMS());
        break;
      }
      // Make sure we don't hang the system by raising IORDY.
      ISA.setIORDY();
      return;
    }
    // If we never receive a read, exit after a while to avoid getting stuck.
    if (LoopCnt == InfiniteLoopCnt) {
#if ENABLE_SERIAL_DBG == 1
      std::cerr << "InfiniteLoop!\n";
      ISA.clearIRQ();
      sleep_ns(100);
#endif
      return;
    }
  }
}

static Pico *PicoPtr = nullptr;

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                const uint8_t *report, uint16_t len) {
  static bool LedState = true;
  TuhHidReceiveReportGuard ReceiveReportGuard(dev_addr, instance);
  if (tuh_hid_interface_protocol(dev_addr, instance) ==
      HID_ITF_PROTOCOL_MOUSE) {
    // Populate MouseState with the new mouse state.
    MouseState.import(reinterpret_cast<const hid_mouse_report_t *>(report));
#if ENABLE_SERIAL_DBG == 1
    printf("mouse data: ");
    MouseState.dump();
#endif

    // Blink LED to show activity.
    LedState = !LedState;
    PicoPtr->ledSet(LedState);

    // Send data
    for (uint32_t MousePacketIdx = 0; MousePacketIdx != 3; ++MousePacketIdx)
      sendMousePacket(*ISA, MousePacketIdx);
#if ENABLE_SERIAL_DBG == 1
    std::cerr << "Sent packets\n";
#endif
  }
}

static void main_loop(ISABus &ISA) {
  PicoPtr = &ISA.getPico();

  // TinyUSB seems to be flaky so enable watchdog.
  // We must have updated the watchdong within 1000ms.
  watchdog_enable(1000, 1);

  auto LastTime = std::chrono::high_resolution_clock::now();
  // Light up LED
  ISA.getPico().ledON();
  while (true) {
    auto NewTime = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(NewTime -
                                                              LastTime)
        .count() >= USB_POLL_PERIOD) {
      LastTime = NewTime;
      tuh_task();
      // Update the watchdog to notify it that we are alive.
      watchdog_update();
    }
  }
}

// Hack for linking error: undefined reference to `__dso_handle'
static void *__dso_handle = 0;

int main() {
  (void)__dso_handle;
  ISA = new ISABus(
      /*Addr=*/{0, 11}, /*Data=*/{15, 22}, /*IRQ=*/12, /*MemR=*/13,
      /*MemW=*/14, /*IOR=*/26, /*IOW=*/27, /*IO_RDY=*/28,
      /*Frequency=*/PICO_FREQ);
  static_assert(
      ENABLE_SERIAL_DBG == 0 || (IO_ADDR & 0b11) == 0,
      "*** When serial debugging is enabled the last 2 bits of the IO_ADDR are "
      "used for serial communication and are always set to 0. "
      "Please use an IO_ADDR with 0 in the two least significant bits!");
#if ENABLE_SERIAL_DBG == 1
  std::cerr << "IO_ADDR=0x" << std::hex << IO_ADDR << "\n";
#endif
  tusb_init();
  main_loop(*ISA);
  return 0;
}
