#!/bin/bash

rm qcfw.crc32

cp -a ../BadWDSD-Stage/Stagex.bin Stagex.bin || exit 1
Stagex_crc32=$(crc32 Stagex.bin)
echo -n $Stagex_crc32 | xxd -r -p >> qcfw.crc32 || exit 1

cp -a ../BadWDSD-Stage/Stagex_aux.bin Stagex_aux.bin || exit 1
Stagex_aux_crc32=$(crc32 Stagex_aux.bin)
echo -n $Stagex_aux_crc32 | xxd -r -p >> qcfw.crc32 || exit 1

Coreos_crc32=$(crc32 CoreOS.bin)
echo -n $Coreos_crc32 | xxd -r -p >> qcfw.crc32 || exit 1