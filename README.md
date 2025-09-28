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

## How to make fanreg a Linux daemon?
1. **Create service-file**
```bash
sudo tee /etc/systemd/system/fanreg.service <<EOF
[Unit]
Description=Daemon for monitoring temperature of IBM laptops
After=multi-user.target

[Service]
ExecStart=/usr/local/bin/fanreg
Restart=always
RestartSec=5
User=root

[Install]
WantedBy=multi-user.target
EOF
```
2.**Startup the deamon **
```bash
sudo systemctl daemon-reload
sudo systemctl start fanreg
sudo systemctl enable fanreg
```
  
  
   
   
## Warning
It's projects in BETA!


