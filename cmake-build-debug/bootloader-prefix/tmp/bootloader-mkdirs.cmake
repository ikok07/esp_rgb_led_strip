# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/kok/esp/esp-idf/components/bootloader/subproject"
  "/home/kok/CLionProjects/esp_rgb_led_strip/cmake-build-debug/bootloader"
  "/home/kok/CLionProjects/esp_rgb_led_strip/cmake-build-debug/bootloader-prefix"
  "/home/kok/CLionProjects/esp_rgb_led_strip/cmake-build-debug/bootloader-prefix/tmp"
  "/home/kok/CLionProjects/esp_rgb_led_strip/cmake-build-debug/bootloader-prefix/src/bootloader-stamp"
  "/home/kok/CLionProjects/esp_rgb_led_strip/cmake-build-debug/bootloader-prefix/src"
  "/home/kok/CLionProjects/esp_rgb_led_strip/cmake-build-debug/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/kok/CLionProjects/esp_rgb_led_strip/cmake-build-debug/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/kok/CLionProjects/esp_rgb_led_strip/cmake-build-debug/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
