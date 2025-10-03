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
2. **Creating a client script for the fan control daemon**
 ```bash
  #!/bin/bash
TMP_CONF="/etc/fanreg.conf.tmp"
CONF="/etc/fanreg.conf"
PIDFILE="/var/run/fanreg.pid"

echo "param=$1" > "$TMP_CONF"


if [ ! -s "$TMP_CONF" ]; then
  echo "Еrror: config is empty"
  exit 1
fi

mv "$TMP_CONF" "$CONF"

if [ -f "$PIDFILE" ]; then
 kill -SIGHUP $(cat "$PIDFILE")
else
 echo "Is the fanreg-daemon is not running?"
fi
````
Don't forget to create etc/fanreg.conf file
3.**Startup the deamon **
```bash
sudo systemctl daemon-reload
sudo systemctl start fanreg
sudo systemctl enable fanreg
```

## How to use?
```bash
fanreg <mode>
```
mode:
med - for routine work 
ext - for routine work with workload
max - for workload

## Warning
It's projects in BETA!
Please review the code before running the program!



