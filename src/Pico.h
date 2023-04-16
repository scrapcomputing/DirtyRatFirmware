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

#ifndef __SRC_PICO_H__
#define __SRC_PICO_H__

#include "Utils.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "pico/stdlib.h"
#include <iostream>
#include <optional>

/// A range of GPIO pins. This is useful because it also creates the
/// corresponding mask which is used for setting and resetting pins.
class PinRange {
  uint32_t From = 0;
  uint32_t To = 0;
  uint32_t Mask = 0;

public:
  /// \p From is the first pin and \p To is the last pin in the reange.
  PinRange(uint32_t From, uint32_t To);
  /// Use this constructor to create a single pin.
  PinRange(uint32_t Pin) : PinRange(Pin, Pin) {}
  uint32_t getFrom() const { return From; }
  uint32_t getTo() const { return To; }
  uint32_t getMask() const { return Mask; }
  void dump(std::ostream &OS) const;
  DUMP_METHOD void dump() const;
};

class Pico {
public:
  // Some valid frequencies: 225000, 250000, 270000, 280000, 290400
  // Some voltages: VREG_VOLTAGE_1_10, VREG_VOLTAGE_1_15
  Pico(uint32_t Frequency = 125000,
       std::optional<vreg_voltage> Voltage = std::nullopt);
  /// Sets \p Pins to GPIO_OUT.
  inline void setGPIODirectionOut(const PinRange &Pins) {
    gpio_set_dir_out_masked(Pins.getMask());
  }
  /// Sets \p Pins to GPIO_IN.
  inline void setGPIODirectionIn(const PinRange &Pins) {
    gpio_set_dir_in_masked(Pins.getMask());
  }
  /// Sets \p Pins to \p Value.
  inline void setGPIOBits(const PinRange &Pins, uint32_t Value) {
    gpio_put_masked(Pins.getMask(), Value << Pins.getFrom());
  }
  /// Direction can be GPIO_IN GPIO_OUT.
  void initGPIO(const PinRange &Pins, int Direction, const char *Descr);
  inline uint32_t readGPIO() const { return gpio_get_all(); }
  void ledSet(bool State) { gpio_put(PICO_DEFAULT_LED_PIN, State); }
  void ledON() { gpio_put(PICO_DEFAULT_LED_PIN, 1); }
  void ledOFF() { gpio_put(PICO_DEFAULT_LED_PIN, 0); }
  inline void clear(uint32_t Mask) { gpio_clr_mask(Mask); }
  inline void set(uint32_t Mask) { gpio_set_mask(Mask); }
};

#endif // __SRC_PICO_H__

