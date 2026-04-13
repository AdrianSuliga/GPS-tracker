cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(GPS-tracker)

zephyr_include_directories(inc)

target_sources(app PRIVATE 
    src/main.c
    src/lte_common.c
    src/coap_common.c
    src/gnss_common.c)
