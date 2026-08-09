.global stage_entry

stage_entry:
    bl 4
    mflr %r3
    addi %r3, %r3, -4
    ld %r4, 24(%r3)
    mtctr %r4
    bctr

    .quad 0x2401FF21000