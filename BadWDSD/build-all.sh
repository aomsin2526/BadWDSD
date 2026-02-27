#!/bin/bash

rm -rf out
mkdir out

#

## Pico
rm -rf BadWDSD/build
##

#

mkdir BadWDSD/build
echo -e '#define PICO_TYPE PICO_TYPE_E_PICO\n#define SC_IS_SW 1\n#define XDR_IS_X32 1' > BadWDSD/build/Config.h || exit 1
./build.sh || exit 1

cp -a BadWDSD/build/BadWDSD.uf2 out/BadWDSD_SW_x32_Pico.uf2 || exit 1

#

mkdir BadWDSD/build
echo -e '#define PICO_TYPE PICO_TYPE_E_RP2040_ZERO\n#define SC_IS_SW 1\n#define XDR_IS_X32 1' > BadWDSD/build/Config.h || exit 1
./build.sh || exit 1

cp -a BadWDSD/build/BadWDSD.uf2 out/BadWDSD_SW_x32_RP2040_Zero.uf2 || exit 1

#

mkdir BadWDSD/build
echo -e '#define PICO_TYPE PICO_TYPE_E_PICO' > BadWDSD/build/Config.h || exit 1
./build.sh || exit 1

cp -a BadWDSD/build/BadWDSD.uf2 out/BadWDSD_CXRF_x16_Pico.uf2 || exit 1

#

## Pico W
rm -rf BadWDSD/build
##

#

mkdir BadWDSD/build
echo -e '#define PICO_TYPE PICO_TYPE_E_PICO_W\n#define SC_IS_SW 1\n#define XDR_IS_X32 1' > BadWDSD/build/Config.h || exit 1
./build.sh -DIS_PICO_W=1 || exit 1

cp -a BadWDSD/build/BadWDSD.uf2 out/BadWDSD_SW_x32_Pico_W.uf2 || exit 1

#