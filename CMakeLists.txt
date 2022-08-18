cmake_minimum_required(VERSION 3.21)
project(dashd C)

set(CMAKE_C_STANDARD 99)

add_executable(dashd main.c n76e_firmware.h)
