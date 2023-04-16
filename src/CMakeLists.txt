cmake_minimum_required(VERSION 3.13)

# For a debug build
# -----------------
# $ mkdir build && cd build && cmake -DENABLE_SERIAL_DBG=1 ../src/ && make -j
#
# This will place the firmware into: build/DirtyRatFirmware.uf2
#
# This build uses USB CNC instead of the Pico's UART pins.
# This means that as long as the Pico is connected to your PC via USB
# you can connect to it using /dev/ttyACM0.
# For example: minicom -b 115200 -D /dev/ttyACM0. Serial connection: 115200 8N1


set(PICO_SDK_PATH "$ENV{HOME}/pico-sdk/")

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(DirtyRatFirmware)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()
include_directories("${PICO_SDK_PATH}/src/common/pico_stdlib/include")
include_directories("${PICO_SDK_PATH}/src/common/pico_base/include")
include_directories("${PROJECT_BINARY_DIR}/") # for build/config.h

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Werror ${EXTRA_DBG_FLAGS}")

# rest of your project
file(GLOB SOURCES *.c *.cpp *.h *.def)
add_executable(DirtyRatFirmware ${SOURCES})

target_link_libraries(DirtyRatFirmware pico_stdlib tinyusb_host tinyusb_board tinyusb_device)
target_include_directories(DirtyRatFirmware PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

set_property(TARGET DirtyRatFirmware PROPERTY CXX_STANDARD 17)
set_property(TARGET DirtyRatFirmware PROPERTY CXX_STANDARD_REQUIRED ON)

# # Load program from RAM
# pico_set_binary_type(DirtyRatFirmware no_flash)

message("")
message("+---------------+")
message("| Configuration |")
message("+---------------+")

# Use -DENABLE_SERIAL_DBG=1 to enable debug messages in serial debugging
if (ENABLE_SERIAL_DBG STREQUAL "1")
  pico_enable_stdio_usb(DirtyRatFirmware 0)
  pico_enable_stdio_uart(DirtyRatFirmware 1)
else ()
  set(ENABLE_SERIAL_DBG 0)
  pico_enable_stdio_usb(DirtyRatFirmware 0)
  pico_enable_stdio_uart(DirtyRatFirmware 0)
endif ()
message("ENABLE_SERIAL_DBG = ${ENABLE_SERIAL_DBG}")

# Use -DIO_ADDR to override the default IO_ADDR 0x2e8
if (NOT DEFINED IO_ADDR)
  set(IO_ADDR 0x2e8)
endif ()
message("IO_ADDR = ${IO_ADDR}")

# Use -DPICO_FREQ to override the default frequency 133000
# *** Please note that not all frequency values are valid
#     Some good values: 125000 133000 225000 270000
if (NOT DEFINED PICO_FREQ)
  set(PICO_FREQ 133000)
endif ()
message("PICO_FREQ = ${PICO_FREQ} (KHz)")

# Use -DUSB_POLL_PERIOD to set the USB polling period in milliseconds.
# The default is 25ms which corresponds to 40Hz
if (NOT DEFINED USB_POLL_PERIOD)
  set(USB_POLL_PERIOD 25)
endif ()
message("USB_POLL_PERIOD = ${USB_POLL_PERIOD} (ms)")


# End of configuration
message("")

configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

# Create map/bin/hex/uf2 in addition to ELF.
pico_add_extra_outputs(DirtyRatFirmware)
