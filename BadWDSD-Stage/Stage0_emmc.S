.global stage_entry

stage_entry:
    // r10 = sb_uart_stat

    lis %r10, 0x240
    ori %r10, %r10, 0xff
    sldi %r10, %r10, 16
    ori %r10, %r10, 0xf308 // sb_uart_stat

    li %r3, 0x7a // 'z'
    stw %r3, 20(%r10) // sb_uart_putchar('z')

    // r9 = cur addr
    li %r9, 0x1000

loop1:
    // read stat
    lwz %r3, 0(%r10)
    andi. %r3, %r3, 0x1f
    beq loop1

    // getc
    lwz %r3, 24(%r10)

    // write lowest 8 bit of r3 to address at r9
    stb %r3, 0(%r9)
    addi %r9, %r9, 1 // r9 += 1

    // loop if r9 < 0x2000
    cmpi 0, 0, %r9, 0x2000
    bne loop1

    // jump to 0x1000
    b 0xec4