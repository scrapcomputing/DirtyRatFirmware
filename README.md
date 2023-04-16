# DirtyRat Firmware for the ISA Blaster

This is the firmware that needs to be loaded to the Raspberry Pi Pico of the [ISA Blaster](https://github.com/scrapcomputing/ISABlaster) to get it to work as a USB Mouse adapter.
The DOS driver for this is the [DirtyRat Driver](https://github.com/scrapcomputing/DirtyRatDriver).

# Download

Binaries are available in the releases: https://github.com/scrapcomputing/DirtyRatFirmware/releases
The file to be loaded to the Raspberry Pi Pico is the one with the `.uf2` file extension.

# How to use

- The firmware is configured to use `0x2e8` I/O address by default, which usually overlaps with the 2nd serial port COM4. So please make sure that you disable the conflicting port on your PC.
- Load the firmware to the Pico (see instructions below)


# How to load the firmware

- Unplug the ISA Blaster from the motherboard
- Remove any connected USB devices (e.g., mouse)
- Press and hold the small button on the Pico
- While holding the button, connect the Pico to your PC with a micro-USB cable
- The Pico should show up as a mass-storage device
- Copy the `.uf2` firmware to the drive associated with the Pico
- Safely eject the mass-storage device

# Build

## Dependencies:
- [Pico SDK](https://github.com/raspberrypi/pico-sdk) 1.5.0. Please adjust the `PICO_SDK_PATH` in `src/CMakeLists.txt`
- C++ 17 or later compiler
- CMake 3.13 or later
- Recent version of `make` or other build system supported by `CMake`

## Build instructions
- Create a `build` directory and cd into it: `mkdir build && cd build`
- `cmake ../src/`
- `make`
The firmware should be in `build/DirtyRatFirmware.uf2`.

## Build Options
You can override some of the default firmware options:
- Specify the I/O Address with `cmake -DIO_ADDR=` (Default is `0x2e8`)
- Specify the Pico CPU Frequency with `-DPICO_FREQ=` (Default is `133000`)
- Specify the USB poll period in milliseconds with `-DUSB_POLL_PERIOD=` (Default is `25`)

For serial debugging the Pico you need to specify `-DENABLE_SERIAL_DBG=1`.
You can use the Pico's GPIO pins 0 and 1 for serial debugging.
The caveat is that these pins are normally address bits 0 and 1, so if you enable serial debugging these two address bits will be forced to 0.


# ISA to Pico Pinout

This table shows the mapping between the ISA bus pins and the Pico's GPIOs.

ISA   | GPIO | Pin
------|------|----
A00   | GP00 | 01
A01   | GP01 | 02
A02   | GP02 | 04
A03   | GP03 | 05
A04   | GP04 | 06
A05   | GP05 | 07
A06   | GP06 | 09
A07   | GP07 | 10
A08   | GP08 | 11
A09   | GP09 | 12
A10   | GP10 | 14
A11   | GP11 | 15
IRQ   | GP12 | 16
MEMR- | GP13 | 17
MEMW- | GP14 | 19
DB0   | GP15 | 20
DB1   | GP16 | 21
DB2   | GP17 | 22
DB3   | GP18 | 24
DB4   | GP19 | 25
DB5   | GP20 | 26
DB6   | GP21 | 27
DB7   | GP22 | 29
IOR-  | GP26 | 31
IOW-  | GP27 | 32
IO_RDY| GP28 | 34


Since the default configuration is using COM4's I/O Address and IRQ, here is a reminder of the serial port addresses and IRQs.

COM |I/O Addr | IRQ
----|---------|-----
COM1| 0x3F8   | 4
COM2| 0x2F8   | 3
COM3| 0x3E8   | 4
COM4| 0x2E8   | 3
