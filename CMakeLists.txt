cmake_minimum_required(VERSION 3.20.0)
set(BOARD rpi_pico)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openzirk)


target_include_directories(app PRIVATE inc)
target_sources(app PRIVATE src/main.c src/current_sense.c src/temp_sense.c src/io.c)
