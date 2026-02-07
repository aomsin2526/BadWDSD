#if 0

.text:0000000080002D0C loc_80002D0C:                           # CODE XREF: sub_80002ACC+224↑j
.text:0000000080002D0C                 ld        r5, off_C0006C30 # "SL:auth_lv2 ret_value %d\n"
.text:0000000080002D10                 li        r4, 2
.text:0000000080002D14                 ld        r3, off_C0006B00 # stru_C00030F8
.text:0000000080002D18                 extsw     r6, r31
.text:0000000080002D1C                 bl        sub_80008648
.text:0000000080002D20                 b         loc_80002D28
.text:0000000080002D24 # ---------------------------------------------------------------------------
.text:0000000080002D24
.text:0000000080002D24 loc_80002D24:                           # CODE XREF: sub_80002ACC+44↑j
.text:0000000080002D24                                         # sub_80002ACC+58↑j ...
.text:0000000080002D24                 li        r31, 0xF

#endif

// size must be 28
// r28 = laid
// r31 = return value (to r3)

.global stage_entry

stage_entry:
    li %r4, 0x6996
    li %r5, 0x30
    mr %r6, %r28

    // r11 = 0x10030
    addis %r11, %r5, 0x1

    sc

    nop
    nop