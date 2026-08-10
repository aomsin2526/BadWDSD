#!/bin/bash

rm qcfw.crc32

cp -a ../BadWDSD-Stage/Stagex.bin Stagex.bin || exit 1
Stagex_size=`stat --printf="%s" Stagex.bin`
Stagex_size_hex=$(printf '%08x' $Stagex_size)
Stagex_crc32=$(crc32 Stagex.bin)
echo -n $Stagex_crc32 | xxd -r -p >> qcfw.crc32 || exit 1

cp -a ../BadWDSD-Stage/Stagex_aux.bin Stagex_aux.bin || exit 1
Stagex_aux_size=`stat --printf="%s" Stagex_aux.bin`
Stagex_aux_size_hex=$(printf '%08x' $Stagex_aux_size)
Stagex_aux_crc32=$(crc32 Stagex_aux.bin)
echo -n $Stagex_aux_crc32 | xxd -r -p >> qcfw.crc32 || exit 1

Coreos_crc32=$(crc32 CoreOS.bin)
echo -n $Coreos_crc32 | xxd -r -p >> qcfw.crc32 || exit 1

# modchip v2
echo -n 00 | xxd -r -p >> qcfw.crc32 || exit 1

#

rm qcfw_sc_eeprom.txt

printf "Run these syscon command after hardware flashing:\n\n" > qcfw_sc_eeprom.txt

printf "w fa0 ${Stagex_size_hex:0:2} ${Stagex_size_hex:2:2} ${Stagex_size_hex:4:2} ${Stagex_size_hex:6:2}\n" >> qcfw_sc_eeprom.txt
printf "w fa4 ${Stagex_crc32:0:2} ${Stagex_crc32:2:2} ${Stagex_crc32:4:2} ${Stagex_crc32:6:2}\n" >> qcfw_sc_eeprom.txt

printf "w fa8 ${Stagex_aux_size_hex:0:2} ${Stagex_aux_size_hex:2:2} ${Stagex_aux_size_hex:4:2} ${Stagex_aux_size_hex:6:2}\n" >> qcfw_sc_eeprom.txt
printf "w fac ${Stagex_aux_crc32:0:2} ${Stagex_aux_crc32:2:2} ${Stagex_aux_crc32:4:2} ${Stagex_aux_crc32:6:2}" >> qcfw_sc_eeprom.txt