#if 0

seg002:00000000002B3684 SPU_iso_load_request:                   # CODE XREF: sub_2B56CC+28↓p
seg002:00000000002B3684                 ld        r9, -0x30E0(r2)
seg002:00000000002B3688                 sldi      r3, r3, 19
seg002:00000000002B368C                 li        r0, 3
seg002:00000000002B3690                 ld        r9, 0(r9)
seg002:00000000002B3694                 addis     r9, r9, 4
seg002:00000000002B3698                 addi      r9, r9, 0x401C
seg002:00000000002B369C                 stwx      r0, r3, r9
seg002:00000000002B36A0                 blr
seg002:00000000002B36A0 # End of function SPU_iso_load_request

E9 22 CF 20 78 63 9B 24 38 00 00 03 E9 29 00 00 3D 29 00 04 39 29 40 1C 7C 03 49 2E

seg002:00000000002B312C __get_spu_status:                       # CODE XREF: j___get_spu_status↓j
seg002:00000000002B312C                                         # sub_2B55D4+34↓p
seg002:00000000002B312C                 eieio
seg002:00000000002B3130                 ld        r9, -0x30E0(r2)
seg002:00000000002B3134                 sldi      r3, r3, 19
seg002:00000000002B3138                 ld        r9, 0(r9)
seg002:00000000002B313C                 addis     r9, r9, 4
seg002:00000000002B3140                 addi      r9, r9, 0x4024
seg002:00000000002B3144                 lwzx      r3, r3, r9
seg002:00000000002B3148                 blr
seg002:00000000002B3148 # End of function __get_spu_status

7C 00 06 AC E9 22 CF 20 78 63 9B 24 E9 29 00 00 3D 29 00 04 39 29 40 24 7C 63 48 2E

seg002:00000000002B3664 request_exit_isolation:                 # CODE XREF: sub_2B55D4+40↓p
seg002:00000000002B3664                 ld        r9, -0x30E0(r2)
seg002:00000000002B3668                 sldi      r3, r3, 19
seg002:00000000002B366C                 li        r0, 2
seg002:00000000002B3670                 ld        r9, 0(r9)
seg002:00000000002B3674                 addis     r9, r9, 4
seg002:00000000002B3678                 addi      r9, r9, 0x401C
seg002:00000000002B367C                 stwx      r0, r3, r9
seg002:00000000002B3680                 blr
seg002:00000000002B3680 # End of function request_exit_isolation

E9 22 CF 20 78 63 9B 24 38 00 00 02 E9 29 00 00 3D 29 00 04 39 29 40 1C 7C 03 49 2E

seg002:0000000000319CDC SPE_update_SPU_status_and_transition_notifier_in_shadow_reg_area:
seg002:0000000000319CDC                                         # CODE XREF: sub_316C88+170↑p
seg002:0000000000319CDC                                         # sub_316FA4+1D8↑p ...
seg002:0000000000319CDC                 ld        r10, 0(r3)
seg002:0000000000319CE0                 ld        r11, 8(r3)
seg002:0000000000319CE4                 ld        r9, 8(r10)
seg002:0000000000319CE8                 addis     r9, r9, 4
seg002:0000000000319CEC                 lwz       r0, 0x4024(r9)
seg002:0000000000319CF0                 stw       r0, 0x30(r11)
seg002:0000000000319CF4                 ld        r9, 0xF10(r11)
seg002:0000000000319CF8                 or        r4, r4, r9
seg002:0000000000319CFC                 std       r4, 0xF10(r11)
seg002:0000000000319D00                 blr
seg002:0000000000319D00 # End of function SPE_update_SPU_status_and_transition_notifier_in_shadow_reg_area

E9 43 00 00 E9 63 00 08 E9 2A 00 08 3D 29 00 04 80 09 40 24 90 0B 00 30 E9 2B 0F 10 7C 84 4B 78 F8 8B 0F 10

#endif

// 0x2401F031700

stage_entry:
    li %r10, 0x69 // 1 = SPU_iso_load_request, 2 = __get_spu_status, 3 = request_exit_isolation, 4 = SPE_update_SPU_status_and_transition_notifier_in_shadow_reg_area

    li %r11, 0x220
    ld %r11, 0(%r11)
    mtctr %r11
    bctr