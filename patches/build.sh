#!/bin/bash

export FLAGS="-g -O1 -Wall -mcpu=cell -mabi=elfv1 -ffreestanding -mtoc -nostdlib -Wl,--build-id=none -static"
export CC=powerpc64-linux-gnu-gcc

$CC $FLAGS -T ld.ld disable_erase_hash_standby_bank_and_fsm.S -o disable_erase_hash_standby_bank_and_fsm.elf || exit 1
$CC $FLAGS -T ld.ld disable_erase_hash_standby_bank_and_fsm.S -o disable_erase_hash_standby_bank_and_fsm.bin -Wl,--oformat=binary || exit 1

$CC $FLAGS -T ld.ld patch_get_applicable_version.c -o patch_get_applicable_version.elf || exit 1
$CC $FLAGS -T ld.ld patch_get_applicable_version.c -o patch_get_applicable_version.bin -Wl,--oformat=binary || exit 1

$CC $FLAGS -T ld.ld patch_get_version_and_hash.c -o patch_get_version_and_hash.elf || exit 1
$CC $FLAGS -T ld.ld patch_get_version_and_hash.c -o patch_get_version_and_hash.bin -Wl,--oformat=binary || exit 1

$CC $FLAGS -T ld.ld patch_fsm.S -o patch_fsm.elf || exit 1
$CC $FLAGS -T ld.ld patch_fsm.S -o patch_fsm.bin -Wl,--oformat=binary || exit 1
