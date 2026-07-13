#!/bin/bash

# qcfwgen0.sh <work_dir>
# work_dir must be in same directory as this file

# contents of <work_dir> must be:

# inros.bin (OFW)

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

cd $ROOT_DIR || exit 1
cd $WORK_DIR || exit 1

echo Delete workdir temp...
rm -rf temp

rm outros.bin
rm CoreOS.bin

echo Delete workdir inros...
rm -rf inros

echo Delete workdir outros...
rm -rf outros

echo Copying needed files to temp...
mkdir temp || exit 1

cp $ROOT_DIR/tools/coreos_tools/coreos_tools temp/coreos_tools || exit 1

echo Extracting inros.bin...
mkdir inros

temp/coreos_tools extract_coreos inros.bin inros || exit 1

echo Copying inros to outros...
cp -a inros outros || exit 1

echo Renaming lv1.self to lv1.qelf...
mv outros/lv1.self outros/lv1.qelf || exit 1

echo Deleting creserved_0...
rm outros/creserved_0

echo Creating outros/qcfw_jig
echo "qcfw_jig" > outros/qcfw_jig || exit 1

echo Creating outros/sdk_version to 446.000
echo "446.000" > outros/sdk_version || exit 1

read -p "Modify outros now then press ENTER to continue"

echo Generate CoreOS.bin...
temp/coreos_tools create_coreos outros CoreOS.bin || exit 1

../qcfwgen_finalize.sh || exit 1