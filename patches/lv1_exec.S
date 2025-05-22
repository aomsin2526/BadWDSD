.global patches

// r9 = addr

patches:
    addi %r1, %r1, -16

    mflr %r0
    std %r0, 0(%r1)

    addi %r1, %r1, -128
    mtctr %r9
    bctrl
    addi %r1, %r1, 128

    ld %r0, 0(%r1)
    mtlr %r0

    addi %r1, %r1, 16

    blr