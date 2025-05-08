#!/bin/bash

export FLAGS="-g -O1 -Wall -mcpu=cell -mabi=elfv1 -ffreestanding -mtoc -nostdlib -Wl,--build-id=none -static"

powerpc64-linux-gnu-gcc $FLAGS -T ld.ld disable_erase_hash_standby_bank.S -o disable_erase_hash_standby_bank.elf || exit 1
powerpc64-linux-gnu-gcc $FLAGS -T ld.ld disable_erase_hash_standby_bank.S -o disable_erase_hash_standby_bank.bin -Wl,--oformat=binary || exit 1

powerpc64-linux-gnu-gcc $FLAGS -T ld.ld patch_get_applicable_version.c -o patch_get_applicable_version.elf || exit 1
powerpc64-linux-gnu-gcc $FLAGS -T ld.ld patch_get_applicable_version.c -o patch_get_applicable_version.bin -Wl,--oformat=binary || exit 1

powerpc64-linux-gnu-gcc $FLAGS -T ld.ld patch_get_version_and_hash.c -o patch_get_version_and_hash.elf || exit 1
powerpc64-linux-gnu-gcc $FLAGS -T ld.ld patch_get_version_and_hash.c -o patch_get_version_and_hash.bin -Wl,--oformat=binary || exit 1