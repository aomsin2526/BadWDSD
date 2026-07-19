// run at 0x180 (second ch)
// max size = 128

#define PAYLOAD_HEADER_SIZE 8
#define JUMP_ADDRESS 0x1000

.global stage0b_emmc_entry

stage0b_emmc_entry:
    // restore XDR

    lis %r3, 0x400
    ori %r3, %r3, 0x205

    lis %r4, 0x200
    ori %r4, %r4, 0x50
    sldi %r4, %r4, 16
    ori %r4, %r4, 0xA108

    stw %r3, 0(%r4)
    eieio
    sync

    // wait for it to finish
    lwz %r3, 8(%r4)
    eieio
    sync

    // r10 = sb_uart_stat

    lis %r10, 0x240
    ori %r10, %r10, 0xff
    sldi %r10, %r10, 16
    ori %r10, %r10, 0xf308 // sb_uart_stat

    // r9 = JUMP_ADDRESS, will jump to
    li %r9, JUMP_ADDRESS
    mtctr %r9

    //
    li %r3, 'b'
    stw %r3, 20(%r10) // sb_uart_putchar((uint8_t)r3)

loop1:
    // read stat
    lwz %r3, 0(%r10)
    andi. %r3, %r3, 0x1f
    beq loop1

    // getc
    lwz %r3, 24(%r10)
    andi. %r3, %r3, 0xff

    // write lowest 8 bit of r3 to address at r9
    stb %r3, -PAYLOAD_HEADER_SIZE(%r9)
    addi %r9, %r9, 1 // r9 += 1

    // loop if r9 < (JUMP_ADDRESS + PAYLOAD_SIZE)
    cmpi 0, 0, %r9, (JUMP_ADDRESS + PAYLOAD_SIZE)
    bne loop1

    //
    li %r3, 'c'
    stw %r3, 20(%r10) // sb_uart_putchar((uint8_t)r3)

    // jump to JUMP_ADDRESS
    bctr