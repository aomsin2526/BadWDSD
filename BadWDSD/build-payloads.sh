#!/bin/bash

export FLAGS="-g -O1 -fno-jump-tables -Wall -mcpu=cell -mabi=elfv1 -ffreestanding -mtoc -nostdlib -Wl,--build-id=none -static"

export FLAGS="$FLAGS"
echo FLAGS = $FLAGS

export CC=powerpc64-linux-gnu-gcc
export OBJCOPY=powerpc64-linux-gnu-objcopy

export STAGEXLDR_FLAGS="-estagexldr_link_entry -ffunction-sections -fdata-sections -Wl,--gc-sections"

#

$CC $FLAGS $STAGEXLDR_FLAGS -T Stagexldr_emmc.ld Stagexldr_emmc.c -o Stagexldr_emmc.elf || exit 1
$OBJCOPY -O binary Stagexldr_emmc.elf Stagexldr_emmc.bin || exit 1

STAGEXLDR_EMMC_BIN_SIZE=`stat --printf="%s" Stagexldr_emmc.bin`
echo STAGEXLDR_EMMC_BIN_SIZE = $STAGEXLDR_EMMC_BIN_SIZE

#

rm Stage0b_emmc.S
echo -e "#define PAYLOAD_SIZE $STAGEXLDR_EMMC_BIN_SIZE\n" > Stage0b_emmc.S
cat Stage0b_emmc.template.S >> Stage0b_emmc.S

$CC $FLAGS -T Stage0b_emmc.ld Stage0b_emmc.S -o Stage0b_emmc.elf || exit 1
$CC $FLAGS -T Stage0b_emmc.ld Stage0b_emmc.S -o Stage0b_emmc.bin -Wl,--oformat=binary || exit 1

STAGE0B_EMMC_BIN_SIZE=`stat --printf="%s" Stage0b_emmc.bin`
echo STAGE0B_EMMC_BIN_SIZE = $STAGE0B_EMMC_BIN_SIZE

#

rm Stage0_emmc.S
echo -e "#define PAYLOAD_SIZE $STAGE0B_EMMC_BIN_SIZE\n" > Stage0_emmc.S
cat Stage0_emmc.template.S >> Stage0_emmc.S

$CC $FLAGS -T Stage0_emmc.ld Stage0_emmc.S -o Stage0_emmc.elf || exit 1
$CC $FLAGS -T Stage0_emmc.ld Stage0_emmc.S -o Stage0_emmc.bin -Wl,--oformat=binary || exit 1

#

bin2c -H Stage0_emmc.bin.c Stage0_emmc.bin || exit 1
bin2c -H Stage0b_emmc.bin.c Stage0b_emmc.bin || exit 1

bin2c -H Stagexldr_emmc.bin.c Stagexldr_emmc.bin || exit 1