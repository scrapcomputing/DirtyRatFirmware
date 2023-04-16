//-*- C++ -*-
//
// The main interface to the ISA bus.
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

#ifndef __SRC_ISA_H__
#define __SRC_ISA_H__

#include "Pico.h"
#include "Utils.h"
#include <config.h>
#include <optional>

/// This class allows us to read/write to the ISA bus.
/// Note: Addr01AsDebugPins == true will use the first two Addr bits for UART
/// debug output. This means that Addr[1:0] will always be 0.
class ISABus {
  Pico PICO;

  PinRange Addr;
  PinRange Data;
  PinRange IRQ;
  PinRange MemR;
  PinRange MemW;
  PinRange IOR;
  PinRange IOW;
  PinRange IORDY;

  uint32_t GPIO;

public:
  ISABus(const PinRange &Addr, const PinRange &Data, const PinRange &IRQ,
         const PinRange &MemR, const PinRange &MemW, const PinRange &IOR,
         const PinRange &IOW, const PinRange &IORDY,
         uint32_t Frequency = 125000,
         std::optional<vreg_voltage> Voltage = std::nullopt)
      : PICO(Frequency, Voltage), Addr(Addr), Data(Data), IRQ(IRQ), MemR(MemR),
        MemW(MemW), IOR(IOR), IOW(IOW), IORDY(IORDY) {
#define INIT(PINS, INOUT) PICO.initGPIO(PINS, INOUT, #PINS)

    // If we use Addr[1:0] for the debug UART, don't initialize the first 2
    // pins.
    PinRange AddrPins(Addr);
#if ENABLE_SERIAL_DBG == 1
    AddrPins = PinRange(Addr.getFrom() + 2, Addr.getTo());
#endif
    INIT(AddrPins, GPIO_IN);

    INIT(Data, GPIO_IN);
    INIT(IRQ, GPIO_IN);
    INIT(MemR, GPIO_IN);
    INIT(MemW, GPIO_IN);
    INIT(IOR, GPIO_IN);
    INIT(IOW, GPIO_IN);
    INIT(IORDY, GPIO_IN);
  }
  inline Pico &getPico() { return PICO; }
  /// Reads the state of the ISA Bus. You must call this before calling any of
  /// the get* functions to get the status of specific bits in the bus.
  inline void readBus() {
    GPIO = PICO.readGPIO();
#if ENABLE_SERIAL_DBG == 1
    GPIO &= ~0b11u;
#endif
  }
  /// \Returns the bus stead read by `readBus()`.
  /// WARNING: This does *not* read the bus!
  inline uint32_t getBus() const { return GPIO; }

#define GET(FIELD) (GPIO & FIELD.getMask()) >> FIELD.getFrom()
  /// WARNING: None of these getter functions actually read the bus!
  ///          This is done for performance reasons as we may nead to check
  ///          multiple values from a single bus read.
  ///          We rely on `readBus()` to actually do a bus read and save the
  ///          state in a variable. This variable is accessed by these functions
  inline uint32_t getAddr() const { return GET(Addr); }
  inline uint32_t getData() const { return GET(Data); }
  inline uint32_t getIRQ() const { return GET(IRQ); }
  inline uint32_t getMemR() const { return GET(MemR); }
  inline uint32_t getMemW() const { return GET(MemW); }
  inline uint32_t getIOR() const { return GET(IOR); }
  inline uint32_t getIOW() const { return GET(IOW); }
  inline uint32_t getIORDY() const { return GET(IORDY); }
  inline void clearIORDY() {
    PICO.setGPIODirectionOut(IORDY);
    PICO.clear(IORDY.getMask());
  }
  inline void setIORDY() {
    PICO.set(IORDY.getMask());
    PICO.setGPIODirectionOut(IORDY);
  }
  inline void setIRQ() {
    PICO.setGPIODirectionOut(IRQ);
    PICO.set(IRQ.getMask());
  }
  inline void clearIRQ() {
    PICO.clear(IRQ.getMask());
    PICO.setGPIODirectionIn(IRQ);
  }

  /// Writes \p DataValue to the data bus.
  inline void writeBusCycle(uint32_t DataValue) {
    // Switch Data pins tou GPIO_OUT
    PICO.setGPIODirectionOut(Data);
    // Write the data to the bus.
    PICO.setGPIOBits(Data, DataValue);
    // There is a miniumum 85ns delay between Data write and IORDY deassert.
    // *** This is very important!!!
    //     Values too low or too high won't work
    //     On the Pentium system 50 to 100 work OK
    sleep_ns(4 * 85);
    // Now that we have written the bus, raise IORDY.
    setIORDY();
    // Keep the data active until IOR becomes high.
    readBus();
    uint32_t MaxAttempts = 1024;
    while (getIOR() == 0 && MaxAttempts-- != 0)
      readBus();
    if (MaxAttempts == 0)
      std::cerr << "MaxAttempts!\n";
    // Detach from the data bus, by switching to GPIO_IN.
    PICO.setGPIODirectionIn(Data);
  }
};

#endif // __SRC_ISA_H__
