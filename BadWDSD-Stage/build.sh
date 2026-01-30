#!/bin/bash

export FLAGS="-g -O1 -Wall -mcpu=cell -mabi=elfv1 -ffreestanding -mtoc -nostdlib -Wl,--build-id=none -static"

export CC=powerpc64-linux-gnu-gcc
export OBJCOPY=powerpc64-linux-gnu-objcopy

$CC $FLAGS -T Stage0.ld Stage0.S -o Stage0.elf || exit 1
$CC $FLAGS -T Stage0.ld Stage0.S -o Stage0.bin -Wl,--oformat=binary || exit 1

export STAGEX_FLAGS="-estage_link_entry -ffunction-sections -fdata-sections -Wl,--gc-sections"

$CC $FLAGS $STAGEX_FLAGS -T Stagex.ld Stagex.c -o Stagex.elf || exit 1
$OBJCOPY -O binary Stagex.elf Stagex.bin || exit 1

$CC $FLAGS -T Stage2j.ld Stage2j.S -o Stage2j.elf || exit 1
$CC $FLAGS -T Stage2j.ld Stage2j.S -o Stage2j.bin -Wl,--oformat=binary || exit 1

$CC $FLAGS -T Stage3j.ld Stage3j.S -o Stage3j.elf || exit 1
$CC $FLAGS -T Stage3j.ld Stage3j.S -o Stage3j.bin -Wl,--oformat=binary || exit 1

$CC $FLAGS -T Stage3ja.ld Stage3ja.S -o Stage3ja.elf || exit 1
$CC $FLAGS -T Stage3ja.ld Stage3ja.S -o Stage3ja.bin -Wl,--oformat=binary || exit 1

$CC $FLAGS -T Stage3jz.ld Stage3jz.S -o Stage3jz.elf || exit 1
$CC $FLAGS -T Stage3jz.ld Stage3jz.S -o Stage3jz.bin -Wl,--oformat=binary || exit 1

$CC $FLAGS -T Stage4j.ld Stage4j.c -o Stage4j.elf || exit 1
$CC $FLAGS -T Stage4j.ld Stage4j.c -o Stage4j.bin -Wl,--oformat=binary || exit 1

$CC $FLAGS -T Stage5j.ld Stage5j.S -o Stage5j.elf || exit 1
$CC $FLAGS -T Stage5j.ld Stage5j.S -o Stage5j.bin -Wl,--oformat=binary || exit 1

$CC $FLAGS -T Stage6j.ld Stage6j.S -o Stage6j.elf || exit 1
$CC $FLAGS -T Stage6j.ld Stage6j.S -o Stage6j.bin -Wl,--oformat=binary || exit 1

cd Aux || exit 1
./build.sh || exit 1

cd .. || exit 1
rm Stagex_aux.bin
cp Aux/Stagex_aux.bin Stagex_aux.bin || exit 1