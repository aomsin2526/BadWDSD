.global stage_entry

#if 0

seg002:00000000002B2C2C # __int64 __fastcall SPE_load_request_appldr(unsigned __int64, __int64, __int64)
seg002:00000000002B2C2C SPE_load_request_appldr:                # CODE XREF: sub_315FA0+414↓p
seg002:00000000002B2C2C                                         # DATA XREF: .opd:0000000000341AC8↓o
seg002:00000000002B2C2C                 ld        r9, __obj_spu_qa # _obj_spu_qa
seg002:00000000002B2C30                 mr        r0, r4
seg002:00000000002B2C34                 mr        r6, r5
seg002:00000000002B2C38                 mr        r4, r3
seg002:00000000002B2C3C                 mr        r5, r0
seg002:00000000002B2C40                 ld        r3, (_obj_spu_qa - 0x361110)(r9) # obj_spu_qa
seg002:00000000002B2C44                 b         .CodeVerifier__authenticate_program_segment # r3
seg002:00000000002B2C44 # End of function SPE_load_request_appldr # r4 = spu_id
seg002:00000000002B2C44                                         # r5 = program_auth_id
seg002:00000000002B2C44                                         # r6 = lpar_auth_id

#endif

#if 0

seg002:00000000002B30B4 sub_2B30B4:                             # CODE XREF: sub_316428+414↓p
seg002:00000000002B30B4                 ld        r9, -0x30F8(r2)
seg002:00000000002B30B8                 mr        r0, r4
seg002:00000000002B30BC                 mr        r6, r5
seg002:00000000002B30C0                 mr        r4, r3
seg002:00000000002B30C4                 mr        r5, r0
seg002:00000000002B30C8                 ld        r3, 0(r9)
seg002:00000000002B30CC                 b         sub_2B5088

// E9 22 CF 08 7C 80 23 78 7C A6 2B 78

#endif

#if 0

seg002:00000000002B308C SPE_load_request_lv2ldr_2:              # CODE XREF: sub_316428+30C↓p
seg002:00000000002B308C                 ld        r9, -0x30F8(r2)
seg002:00000000002B3090                 mr        r0, r3
seg002:00000000002B3094                 mr        r11, r4
seg002:00000000002B3098                 mr        r10, r5
seg002:00000000002B309C                 mr        r7, r6
seg002:00000000002B30A0                 mr        r4, r0
seg002:00000000002B30A4                 ld        r3, 0(r9)
seg002:00000000002B30A8                 mr        r5, r11
seg002:00000000002B30AC                 mr        r6, r10
seg002:00000000002B30B0                 b         sub_2B4EA8
seg002:00000000002B30B0 # End of function SPE_load_request_lv2ldr_2

// E9 22 CF 08 7C 60 1B 78 7C 8B 23 78 7C AA 2B 78 7C C7 33 78

#endif

// 0x2401F031400

stage_entry:
    li %r10, 0x0 // 1 = appldr, 2 = lv2ldr

    li %r11, 0x210
    ld %r11, 0(%r11)
    mtctr %r11
    bctr