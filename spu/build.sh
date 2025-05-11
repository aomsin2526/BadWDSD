#!/bin/bash

/mnt/datastore1/ps3/ps3dev/spu/bin/spu-gcc testspu.S -o testspu.bin -Wl,--oformat=binary -nostdlib -static -emain || exit 1