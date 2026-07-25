.global start

start:
    // sp = 0x10000 - 0x11EFF
    ila sp, 0x11E00

    // jump to main
    br main

    //
    stop 1