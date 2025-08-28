#!/bin/bash

# export PS3DEV=/mnt/datastore1/ps3/ps3dev

echo Building Stagex_spu...

export SPU_CC=$PS3DEV/spu/bin/spu-gcc

export SPU_FLAGS="-O2 -Wall -nostdlib -static"
export SPU_STAGEX_FLAGS="-estart"

$SPU_CC $SPU_FLAGS $SPU_STAGEX_FLAGS -T Stagex_spu.ld Stagex_spu.S Stagex_spu.c -o Stagex_spu.elf || exit 1
$SPU_CC $SPU_FLAGS $SPU_STAGEX_FLAGS -g -T Stagex_spu.ld Stagex_spu.S Stagex_spu.c -o Stagex_spu_debug.elf || exit 1

###

echo Creating Stagex_aux.bin...

export WORK_DIR=$PWD
echo WORK_DIR=$WORK_DIR

export ROOT_DIR=$PWD/../../
echo ROOT_DIR=$ROOT_DIR

rm -rf temp
mkdir temp || exit 1

cd $ROOT_DIR/tools/coreos_tools || exit 1
./build.sh || exit 1

cd $WORK_DIR || exit 1
cp $ROOT_DIR/tools/coreos_tools/coreos_tools temp/coreos_tools || exit 1

mkdir temp/Stagex_aux || exit 1
cp Stagex_spu.elf temp/Stagex_aux || exit 1

temp/coreos_tools create_coreos temp/Stagex_aux Stagex_aux.bin || exit 1