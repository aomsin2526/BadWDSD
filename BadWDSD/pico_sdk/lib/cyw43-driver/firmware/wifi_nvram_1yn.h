// Source: https://github.com/murata-wireless/cyw-fmac-nvram.git
// File: cyfmac43439-sdio.1YN.txt

// Other option: boardflags=0x00404201
#define CYW43_NVRAM_BOARDFLAGS "boardflags=0x00404001\x00"

#define CYW43_NVRAM_BOARDFLAGS3 "boardflags3=0x08000000\x00"

static const uint8_t wifi_nvram_4343[] CYW43_RESOURCE_ATTRIBUTE =
    // NVRAM file for CYW943439WLPTH
    // 2.4 GHz, 20 MHz BW mode

    // The following parameter values are just placeholders, need to be updated.
    "NVRAMRev=$Rev: 726808 $\x00"
    "manfid=0x2d0\x00"
    "prodid=0x0727\x00"
    "vendid=0x14e4\x00"
    "devid=0x43e2\x00"
    "boardtype=0x0887\x00"
    "boardrev=0x1101\x00"
    "boardnum=22\x00"
    "macaddr=00:90:4c:2d:a0:05\x00"
    "sromrev=11\x00"
    CYW43_NVRAM_BOARDFLAGS
    CYW43_NVRAM_BOARDFLAGS3
    "xtalfreq=37400\x00"
    "nocrc=1\x00"
    "ag0=255\x00"
    "aa2g=1\x00"
    "ccode=ALL\x00"

    "pa0itssit=0x20\x00"
    "extpagain2g=0\x00"
    // PA parameters for 2.4GHz, measured at CHIP OUTPUT
    "pa2ga0=-168,6777,-789\x00"
    "AvVmid_c0=0x0,0xc8\x00"
    "AvVmidIQcal=0x2,0xa8\x00"
    "cckpwroffset0=5\x00"

    // PPR params
    "maxp2ga0=74\x00"
    "txpwrbckof=6\x00"
    "cckbw202gpo=0\x00"
    "legofdmbw202gpo=0x88888888\x00"
    "mcsbw202gpo=0xaaaaaaaa\x00"
    "propbw202gpo=0xdd\x00"

    // OFDM IIR :
    "ofdmdigfilttype=18\x00"
    "ofdmdigfilttypebe=18\x00"
    // PAPD mode:
    "papdmode=1\x00"
    "papdvalidtest=1\x00"
    "pacalidx2g=45\x00"
    "papdepsoffset=-30\x00"
    "papdendidx=58\x00"

    // LTECX flags
    "ltecxmux=0\x00"
    "ltecxpadnum=0x0102\x00"
    "ltecxfnsel=0x44\x00"
    "ltecxgcigpio=0x01\x00"

    "il0macaddr=00:90:4c:c5:12:38\x00"
    "wl0id=0x431b\x00"

    "deadman_to=0xffffffff\x00"
    // muxenab: 0x1 for UART enable, 0x10 for HOST WAKE INT enable, 0x11 for both enable
    // Power cycle required if change
    "muxenab=0x11\x00"
    // CLDO PWM voltage settings - 0x4 - 1.1 volt
    //cldo_pwm=0x4

    //VCO freq 326.4MHz
    "spurconfig=0x3\x00"

    //SW based desense - Enable by default
    //i.e using glitich statistics as criteria to change crsmin
    "glitch_based_crsmin=1\x00"

    // Default btc_mode
    "btc_mode=0\x00"
    "bt_default_ant=0\x00"

    //Antenna diversity
    "swdiv_en=1\x00"
    "swdiv_gpio=1\x00"

    "tempbased_duty_cycle_en=0\x00"
    "\x00\x00"
;
