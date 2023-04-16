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

#include "Pico.h"
#include "Utils.h"
#include <iomanip>

#ifndef NDEBUG
static uint32_t AlreadySetMask = 0u;
#endif // NDEBUG

PinRange::PinRange(uint32_t From, uint32_t To) : From(From), To(To) {
  Mask = ((1u << (To + 1 - From)) - 1) << From;
#ifndef NDEBUG
  if ((AlreadySetMask & Mask) != 0u)
    die("Some pins in range ", From, " to ", To, " are already set!");
  AlreadySetMask |= Mask;
#endif // NDEBUG
}

void PinRange::dump(std::ostream &OS) const {
  auto Flags = OS.flags();
  if (From == To)
    OS << std::setw(5) << std::setfill(' ') << From;
  else
    OS << std::setw(2) << From << "-" << std::setw(2) << To;
  OS << " Mask=";
  OS << "0x" << std::setw(8) << std::setfill('0') << std::hex << Mask;
  OS.flags(Flags);
}

void PinRange::dump() const { dump(std::cerr); }

Pico::Pico(uint32_t Frequency, std::optional<vreg_voltage> Voltage) {
  // Set the voltage if required.
  if (Voltage)
    vreg_set_voltage(*Voltage);
  // Some over/underclocking if needed.
  set_sys_clock_khz(Frequency, true);
  // Initialize stdio so that we can print debug messages.
  stdio_init_all();
  // Initialize the Pico LED.
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  // Wait until stdio is initialized.
  std::cerr << "+---------------------------------+\n";
  std::cerr << "|             ISABlaster          |\n";
  std::cerr << "+---------------------------------+\n";
  std::cerr << "clk_sys = " << frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS)
            << "KHz\n";
}

void Pico::initGPIO(const PinRange &Pins, int Direction, const char *Descr) {
  std::cout << "Setting up GPIO " << std::setw(5) << std::setfill(' ') << Descr
            << " ";
  Pins.dump(std::cout);
  std::cout << " " << (Direction == GPIO_IN ? "IN" : "OUT") << "\n";
  for (uint32_t Pin = Pins.getFrom(), E = Pins.getTo(); Pin <= E; ++Pin) {
    gpio_init(Pin);
    gpio_set_dir(Pin, Direction);
  }
}
