#!/bin/bash

rm -rf build
west build -b native_posix_64 . -- -DCMAKE_C_FLAGS="-DNATIVE_POSIX -I../../include/tls_config -DDISPLAY"
echo "Run ./build/zephyr/zephyr.elf --bt-dev=hci0"