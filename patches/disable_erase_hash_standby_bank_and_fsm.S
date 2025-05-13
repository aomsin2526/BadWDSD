.global patches

//.text:0000000000000014                 lis       r9, -0x8000
//.text:0000000000000018                 ori       r9, r9, 0x4124 # 0xFFFFFFFF80004124
//.text:000000000000001C                 clrldi    r9, r9, 32

patches:
    mflr %r0

    addi %r1, %r1, -64
    std %r0, 0(%r1)

    lis %r9, -0x8000
    ori %r9, %r9, 0x4124
    clrldi %r9, %r9, 32

    li %r3, 0xFF
    li %r4, 0xFF

    mtctr %r9
    bctrl

    ld %r0, 0(%r1)
    addi %r1, %r1, 64

    mtlr %r0

    li %r3, 0
    blr