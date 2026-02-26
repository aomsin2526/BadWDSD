.global stage_entry

stage_entry:
    bl 4
    mflr %r5
    addi %r5, %r5, -4
    ld %r5, 24(%r5)
    mtctr %r5
    bctr

    .quad 0x2401F031100