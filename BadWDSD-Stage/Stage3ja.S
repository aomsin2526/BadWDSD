#if 0

.text:000000008000D4D0 loc_8000D4D0:                           # CODE XREF: sub_8000D2E0+1E0↑j
.text:000000008000D4D0                 li        r4, 2
.text:000000008000D4D4                 ld        r5, off_C0007C18 # "PM: free buffer for load_profile\n"
.text:000000008000D4D8                 mr        r3, r28
.text:000000008000D4DC                 bl        .SS_DPRINTF

// 38 80 00 02 E8 A2 89 18 7F 83 E3 78 48 00 F5 E5

#endif

// size must be 16

.global stage_entry

stage_entry:
    li %r4, 0x6996
    li %r5, 0x31

    // r11 = 0x10031
    addis %r11, %r5, 0x1

    sc