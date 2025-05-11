.global stage_entry

// 0x2401F031300

stage_entry:
    li %r11, 0x210
    ld %r11, 0(%r11)
    mtctr %r11
    bctrl
    nop