# IBM/Lenovo Fan Control Daemon

A sophisticated thermal monitoring daemon for IBM/Lenovo laptops that provides intelligent fan control based on temperature sensors.

## Features

- Real-time thermal monitoring of 8 sensors
- Adaptive fan speed control (levels 2-6)
- Graceful signal handling (SIGINT, SIGTERM, SIGTSTP)
- Automatic fallback to system control
- Memory leak free (valgrind verified)

## Requirements

- IBM/Lenovo laptop with ACPI support
- Linux kernel with `/proc/acpi/ibm/` interface
- GCC compiler
- sudo privileges

## Complilation and start
gcc -o fanreg fanreg.c
./fanreg


