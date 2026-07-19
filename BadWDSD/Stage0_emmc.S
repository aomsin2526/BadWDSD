#define PAYLOAD_SIZE 128

#define JUMP_ADDRESS 0x180

.global stage0_emmc_entry

#if 1

stage0_emmc_entry:
    // r10 = sb_uart_stat

    lis %r10, 0x240
    ori %r10, %r10, 0xff
    sldi %r10, %r10, 16
    ori %r10, %r10, 0xf308 // sb_uart_stat

    // r9 = JUMP_ADDRESS, will jump to
    li %r9, JUMP_ADDRESS
    mtctr %r9

    stw %r9, 20(%r10) // sb_uart_putchar((uint8_t)r9)

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

    // loop if r9 < (JUMP_ADDRESS + PAYLOAD_SIZE)
    cmpi 0, 0, %r9, (JUMP_ADDRESS + PAYLOAD_SIZE)
    bne loop1

    // jump to JUMP_ADDRESS
    bctr

#endif

#if 0
// second ch test
stage0_emmc_entry:
    // r10 = sb_uart_stat

    lis %r10, 0x240
    ori %r10, %r10, 0xff
    sldi %r10, %r10, 16
    ori %r10, %r10, 0xf308 // sb_uart_stat

    li %r9, 0x180

    li %r3, 'x'
    stb %r3, 0(%r9)
    lbz %r3, 0(%r9)
    stw %r3, 20(%r10) // putc

    li %r3, 'y'
    stb %r3, 127(%r9)
    lbz %r3, 127(%r9)
    stw %r3, 20(%r10) // putc

    b 0
#endif

#if 0
// test restore XDR
stage0_emmc_entry:
    // restore XDR

    lis %r3, 0x400
    ori %r3, %r3, 0x205

    lis %r4, 0x200
    ori %r4, %r4, 0x50
    sldi %r4, %r4, 16
    ori %r4, %r4, 0xA108

    stw %r3, 0(%r4)
    //nop

    // r10 = sb_uart_stat
    lis %r10, 0x240
    ori %r10, %r10, 0xff
    sldi %r10, %r10, 16
    ori %r10, %r10, 0xf308 // sb_uart_stat

    // putc
    li %r9, 0x100

    li %r3, 'g'
    stb %r3, 0(%r9)
    lbz %r3, 0(%r9)
    stw %r3, 20(%r10) // putc
#endif