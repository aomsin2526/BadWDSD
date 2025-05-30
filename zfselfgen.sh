#!/bin/bash

# generate .zfself
# useful for lv2_kernel or ps2_netemu

# zfselfgen.sh <filename_without_ext>

# example: ./zfselfgen.sh ps2_netemu
# then put ps2_netemu.elf (decrypted) into same directory as this file

if [[ $# -eq 0 ]] ; then
    echo 'missing args'
    exit 1
fi

export ROOT_DIR=$PWD
echo ROOT_DIR=$ROOT_DIR

export FILE_NAME_PREFIX=$1
echo FILE_NAME_PREFIX=$FILE_NAME_PREFIX

cd $ROOT_DIR/tools/zgen || exit 1
./build.sh || exit 1

cd $ROOT_DIR/tools/dtbImage_ps3_bin_to_elf || exit 1
./build.sh || exit 1

cd $ROOT_DIR || exit 1

echo Generate $FILE_NAME_PREFIX.zelf
$ROOT_DIR/tools/zgen/zgen zelf_gen $FILE_NAME_PREFIX.elf $FILE_NAME_PREFIX.zelf || exit 1

echo Generate $FILE_NAME_PREFIX.zzelf
$ROOT_DIR/tools/dtbImage_ps3_bin_to_elf/dtbImage_ps3_bin_to_elf $FILE_NAME_PREFIX.zelf $FILE_NAME_PREFIX.zzelf || exit 1

echo Generate $FILE_NAME_PREFIX.zfself using this command: make_fself -u $FILE_NAME_PREFIX.zzelf $FILE_NAME_PREFIX.zfself
read -p "then press ENTER to continue"

echo Done.
echo Rename zfself file to ps2_netemu.self or lv2_kernel.self if required
