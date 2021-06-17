#!/bin/bash

BLUETOOTH=${1:-0}
rm -rf build
west build -b native_posix_64 . -- -DCMAKE_C_FLAGS="-DNATIVE_POSIX -I../../include/tls_config -DDISPLAY -DBLUETOOTH=$BLUETOOTH"
echo "Run ./build/zephyr/zephyr.elf --bt-dev=hci0"