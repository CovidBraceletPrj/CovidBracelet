#!/bin/bash

rm -rf build
west build -b native_posix_64 . -- -DCMAKE_C_FLAGS="-DNATIVE_POSIX -I../../include/tls_config -DDISPLAY" -DNATIVE_POSIX=1
echo "Run ./build/zephyr/zephyr.elf"