#!/bin/bash

# export PS3DEV=/mnt/datastore1/ps3/ps3dev

export SPU_CC=$PS3DEV/spu/bin/spu-gcc

export SPU_FLAGS="-Os -Wall -nostdlib -static -ffunction-sections -fdata-sections -Wl,--gc-sections -flto"
export SPU_MYAPPLDR_PATCHES_FLAGS="-estart"

$SPU_CC $SPU_FLAGS $SPU_MYAPPLDR_PATCHES_FLAGS -g -T link.ld read_key_ring.c -o read_key_ring.elf || exit 1
$SPU_CC $SPU_FLAGS $SPU_MYAPPLDR_PATCHES_FLAGS -T link.ld read_key_ring.c -o read_key_ring.bin -Wl,--oformat=binary || exit 1

$SPU_CC $SPU_FLAGS $SPU_MYAPPLDR_PATCHES_FLAGS -g -T link.ld protect_key_ring.c -o protect_key_ring.elf || exit 1
$SPU_CC $SPU_FLAGS $SPU_MYAPPLDR_PATCHES_FLAGS -T link.ld protect_key_ring.c -o protect_key_ring.bin -Wl,--oformat=binary || exit 1

$SPU_CC $SPU_FLAGS $SPU_MYAPPLDR_PATCHES_FLAGS -T link.ld eid0.S -o eid0.bin -Wl,--oformat=binary || exit 1

$SPU_CC $SPU_FLAGS $SPU_MYAPPLDR_PATCHES_FLAGS -g -T link.ld decrypt_eid.c -o decrypt_eid.elf || exit 1
$SPU_CC $SPU_FLAGS $SPU_MYAPPLDR_PATCHES_FLAGS -T link.ld decrypt_eid.c -o decrypt_eid.bin -Wl,--oformat=binary || exit 1