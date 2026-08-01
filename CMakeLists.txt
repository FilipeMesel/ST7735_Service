cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(digital_input)

target_sources(app PRIVATE 
               subsys/ui_engine/include
               src/main.c
               subsys/ui_engine/src/ui_engine.c
               subsys/ui_engine/src/ui_layout.c)