.global start

start:
    // sp = 0x30000 - 0x3CEFF
    ila sp, 0x3CE00

    // jump to main
    br main

    //
    stop 1