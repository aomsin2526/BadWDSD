#!/bin/bash

# export PS3DEV=/mnt/datastore1/ps3/ps3dev

export SPU_CC=$PS3DEV/spu/bin/spu-gcc

export SPU_FLAGS="-Wall -static -nostdlib -ffunction-sections -fdata-sections -Wl,--gc-sections -flto"
export SPU_STAGEX_FLAGS="-O2 -estart"
export SPU_MYMETLDR_FLAGS="-Os -estart"

echo Building Stagex_spu...

$SPU_CC $SPU_FLAGS $SPU_STAGEX_FLAGS -T Stagex_spu.ld Stagex_spu.S Stagex_spu.c -o Stagex_spu.elf || exit 1
$SPU_CC $SPU_FLAGS $SPU_STAGEX_FLAGS -g -T Stagex_spu.ld Stagex_spu.S Stagex_spu.c -o Stagex_spu_debug.elf || exit 1

echo Building mymetldr...

$SPU_CC $SPU_FLAGS $SPU_MYMETLDR_FLAGS -T mymetldr.ld mymetldr.S mymetldr.c -o mymetldr.elf || exit 1
$SPU_CC $SPU_FLAGS $SPU_MYMETLDR_FLAGS -g -T mymetldr.ld mymetldr.S mymetldr.c -o mymetldr_debug.elf || exit 1

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
cp mymetldr.elf temp/Stagex_aux || exit 1

###

echo Copying qcfw-lite...

cp ../../qcfw-lite/qcfwlite492cex_lv1.diff temp/Stagex_aux || exit 1
cp ../../qcfw-lite/qcfwlite492cex_lv2_kernel.zdiff temp/Stagex_aux || exit 1

###

temp/coreos_tools create_coreos temp/Stagex_aux Stagex_aux.bin || exit 1