.global start

start:
    // sp = 0x3CF00
    ila sp, 0x3CF00

    // jump to main
    br main

    //
    stop 1