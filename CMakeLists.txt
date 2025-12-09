cmake_minimum_required(VERSION 3.31.6)
project(ProjetLp C)

set(CMAKE_C_STANDARD 11)

add_executable(ProjetLp main.c
        manager.c
        manager.h
        ui.c
        ui.h
        process.c
        process.h)
