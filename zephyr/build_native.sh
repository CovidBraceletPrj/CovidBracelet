#!/bin/bash

rm -rf build
cp ../.pio/libdeps/nrf52840_dk/exposure-notification/*/exposure-notification.* ../src
west build -b native_posix_64 . -- -DCMAKE_C_FLAGS="-DNATIVE_POSIX -I../../include/tls_config -DDISPLAY"
rm ../src/exposure-notification.c ../src/exposure-notification.h
echo "Run ./build/zephyr/zephyr.elf --bt-dev=hci0"