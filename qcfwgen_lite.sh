#!/bin/bash

# qcfwgen0.sh <work_dir>
# work_dir must be in same directory as this file

# contents of <work_dir> must be:

# inros.bin (OFW)

# lv1.elf.orig (OFW)
# lv1.elf (OFW or patched)

# lv2_kernel.elf.orig (OFW)
# lv2_kernel.elf (OFW or patched)

if [[ $# -eq 0 ]] ; then
    echo 'missing args'
    exit 1
fi

export ROOT_DIR=$PWD
echo ROOT_DIR=$ROOT_DIR

export WORK_DIR=$1
echo WORK_DIR=$WORK_DIR

echo Building stage...
cd $ROOT_DIR/BadWDSD-Stage || exit 1
./build.sh || exit 1

echo Building tools...

cd $ROOT_DIR/tools/coreos_tools || exit 1
./build.sh || exit 1

cd $ROOT_DIR/tools/lv1gen || exit 1
./build.sh || exit 1

cd $ROOT_DIR/tools/lv2gen || exit 1
./build.sh || exit 1

cd $ROOT_DIR/tools/zgen || exit 1
./build.sh || exit 1

cd $ROOT_DIR || exit 1
cd $WORK_DIR || exit 1

echo Delete workdir temp...
rm -rf temp

rm lv1.stage3j3ja4j5j6j.elf

echo Delete workdir inros...
rm -rf inros

echo Copying needed files to temp...
mkdir temp || exit 1

cp $ROOT_DIR/BadWDSD-Stage/Stage2j.bin temp/Stage2j.bin || exit 1
cp $ROOT_DIR/BadWDSD-Stage/Stage3j.bin temp/Stage3j.bin || exit 1
cp $ROOT_DIR/BadWDSD-Stage/Stage3ja.bin temp/Stage3ja.bin || exit 1
cp $ROOT_DIR/BadWDSD-Stage/Stage4j.bin temp/Stage4j.bin || exit 1
cp $ROOT_DIR/BadWDSD-Stage/Stage5j.bin temp/Stage5j.bin || exit 1
cp $ROOT_DIR/BadWDSD-Stage/Stage6j.bin temp/Stage6j.bin || exit 1

cp $ROOT_DIR/tools/coreos_tools/coreos_tools temp/coreos_tools || exit 1
cp $ROOT_DIR/tools/lv1gen/lv1gen temp/lv1gen || exit 1
cp $ROOT_DIR/tools/lv2gen/lv2gen temp/lv2gen || exit 1
cp $ROOT_DIR/tools/zgen/zgen temp/zgen || exit 1

echo Extracting inros.bin...
mkdir inros

temp/coreos_tools extract_coreos inros.bin inros || exit 1

echo Install stage3j/3ja/4j/5j/6j to lv1.elf...
temp/lv1gen lv1gen_4j lv1.elf lv1.stage3j3ja4j5j6j.elf temp/Stage3j.bin temp/Stage3ja.bin temp/Stage4j.bin temp/Stage5j.bin temp/Stage6j.bin || exit 1

echo Generate lv1.diff
temp/lv1gen lv1diff lv1.elf.orig lv1.stage3j3ja4j5j6j.elf lv1.diff || exit 1

echo Generate lv2_kernel.diff
temp/lv2gen lv2diff lv2_kernel.elf.orig lv2_kernel.elf lv2_kernel.diff || exit 1

echo Generate lv2_kernel.zdiff
temp/zgen zelf_gen lv2_kernel.diff lv2_kernel.zdiff || exit 1