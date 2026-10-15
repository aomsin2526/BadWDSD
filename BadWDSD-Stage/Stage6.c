struct mymetldr_context_s
{
    uint64_t myldrElfAddress;
    uint64_t fwVersion;
};

FUNC_DEF void Stage6_IsoLoadRequest(uint64_t spu_id)
{
    //lv1_puts("Stage6_IsoLoadRequest()\n");
    
    //lv1_puts("spu_id = ");
    //lv1_print_decimal(spu_id);
    //lv1_puts("\n");

    struct Stagex_Context_s* ctx = GetStagexContext();

    //

    if (ctx->stage6_spu_id == 0xff)
        ctx->stage6_spu_id = (uint8_t)spu_id;

    //

    uint8_t ok = 0;
    const void* myldrElf = NULL;

    if (spu_id == ctx->stage6_spu_id)
    {
        if (ctx->stage6_isLv2ldr || ctx->stage6_isLv2ldr_rvk)
        {
            if (ctx->has_mylv2ldr)
            {
                // load mylv2ldr to sharedLdr
                if (ctx->cached_sharedLdr_CurKind != 3)
                {
                    lv1_puts("load mylv2ldr\n");

                    FlashRead(ctx->cached_mylv2ldrElf_FileFlashOffset, ctx->cached_sharedLdr, ctx->cached_mylv2ldrElf_FileSize);
                    ctx->cached_sharedLdr_CurKind = 3;
                }

                myldrElf = ctx->cached_sharedLdr;
                ok = 1;
            }
            else if (ctx->cached_sharedLdr_CurKind != 1)
            {
                lv1_puts("load lv2ldr\n");

                // load lv2ldr to sharedLdr

                uint8_t* lv0Self = (uint8_t*)0xB000000;
                uint8_t* lv0Elf = (uint8_t*)0xB800000;

                FlashRead(ctx->cached_lv0_FileFlashOffset, lv0Self, ctx->cached_lv0_FileSize);

                uint64_t myspu_id = calc_myspu_id_exclude(spu_id);

                uint32_t old_mfc_sr1 = SpuAux_Init_lv1(myspu_id);
                SPU_DecryptLv0Self(myspu_id, lv0Elf, lv0Self);
                SpuAux_Uninit(myspu_id, old_mfc_sr1);

                memcpy(ctx->cached_sharedLdr, (lv0Elf + 0x8b844), 0x18AEC);
                memcpy((ctx->cached_sharedLdr + 0x210), ctx->cached_lv2ldr_meta, 0x370);

                ctx->cached_sharedLdr_CurKind = 1;
            }
        }

        if (ctx->stage6_isAppldr)
        {
            if (ctx->has_myappldr)
            {
                // load myappldr to sharedLdr
                if (ctx->cached_sharedLdr_CurKind != 4)
                {
                    lv1_puts("load myappldr\n");

                    FlashRead(ctx->cached_myappldrElf_FileFlashOffset, ctx->cached_sharedLdr, ctx->cached_myappldrElf_FileSize);
                    ctx->cached_sharedLdr_CurKind = 4;
                }

                myldrElf = ctx->cached_sharedLdr;
                ok = 1;
            }
            else if (ctx->cached_sharedLdr_CurKind != 2)
            {
                lv1_puts("load appldr\n");

                // load appldr to sharedLdr

                uint8_t* lv0Self = (uint8_t*)0xB000000;
                uint8_t* lv0Elf = (uint8_t*)0xB800000;

                FlashRead(ctx->cached_lv0_FileFlashOffset, lv0Self, ctx->cached_lv0_FileSize);

                uint64_t myspu_id = calc_myspu_id_exclude(spu_id);

                uint32_t old_mfc_sr1 = SpuAux_Init_lv1(myspu_id);
                SPU_DecryptLv0Self(myspu_id, lv0Elf, lv0Self);
                SpuAux_Uninit(myspu_id, old_mfc_sr1);

                memcpy(ctx->cached_sharedLdr, (lv0Elf + 0xb8a20), 0x27A38);
                memcpy((ctx->cached_sharedLdr + 0x210), ctx->cached_appldr_meta, 0x370);

                ctx->cached_sharedLdr_CurKind = 2;
            }
        }

        ctx->stage6_isAppldr = 0;
        ctx->stage6_isLv2ldr = 0;
        ctx->stage6_isLv2ldr_rvk = 0;
    }

    if (!ok)
    {
        SPU_IsoLoadRequest(spu_id);
        return;
    }

    //

    const void* mymetldrElf = ctx->cached_mymetldrElf;

    if (mymetldrElf == NULL)
    {
        lv1_puts("mymetldr.elf not found!\n");
        dead_beep();
    }

    SPU_StopRequest(spu_id);
    SPU_IsoExitRequest(spu_id);

    //lv1_puts("Loading mymetldr.elf...\n");
    LoadElfSpu((uint64_t)mymetldrElf, spu_id, 1);

    {
        struct mymetldr_context_s mctx;
        mctx.myldrElfAddress = (uint64_t)myldrElf;
        mctx.fwVersion = ctx->cached_fwVersion;

        memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &mctx, sizeof(mctx));
    }

    {
        // marker
        SPU_LS_Write64(spu_id, 0x39100, 0x123456789);
    }

    SPU_StartRequest(spu_id);
}

FUNC_DEF uint32_t Stage6_GetSpuStatus(uint64_t spu_id)
{
    //lv1_puts("Stage6_GetSpuStatus()\n");
    
    //lv1_puts("spu_id = ");
    //lv1_print_decimal(spu_id);
    //lv1_puts("\n");

    uint32_t status = SPU_Read_SPU_STATUS(spu_id);

    if (((status & SPU_STATUS_ISOLATED_MASK) == 0) && (SPU_LS_Read64(spu_id, 0x39100) == 0x123456789))
        status |= SPU_STATUS_ISOLATED_MASK;

    return status;
}

FUNC_DEF void Stage6_RequestExitIsolation(uint64_t spu_id)
{
    //lv1_puts("Stage6_RequestExitIsolation()\n");

    {
        uint32_t status = SPU_Read_SPU_STATUS(spu_id);

        if (((status & SPU_STATUS_ISOLATED_MASK) == 0) && (SPU_LS_Read64(spu_id, 0x39100) == 0x123456789))
            SPU_LS_Write64(spu_id, 0x39100, 0x0);
    }

    SPU_DoIsoExitRequest(spu_id);
}

FUNC_DEF void Stage6_UpdateSPUStatusAndTransitionNotifierInShadowRegArea(uint64_t r3_2, uint64_t r4_2)
{
    //lv1_puts("Stage6_UpdateSPUStatusAndTransitionNotifierInShadowRegArea()\n");

    uint64_t x = *(uint64_t*)r3_2;
    uint64_t shadow_addr = *(uint64_t*)(r3_2 + 8);

    uint64_t ls_start_addr = *(uint64_t*)(x + 8);
    
    uint64_t problem_state_addr = (ls_start_addr + 0x40000);
    uint64_t spu_status_addr = (problem_state_addr + 0x4024);

    uint32_t status = *((uint32_t*)spu_status_addr);

    uint8_t found_spu_id = 0;
    uint64_t spu_id;

    for (uint64_t i = 0; i < 8; ++i)
    {
        if (spu_status_addr == SPU_CalcMMIOAddress_PS(i, 0x4024))
        {
            found_spu_id = 1;
            spu_id = i;

            break;
        }
    }

    if (!found_spu_id)
        dead_beep();

    {
        if (((status & SPU_STATUS_ISOLATED_MASK) == 0) && (SPU_LS_Read64(spu_id, 0x39100) == 0x123456789))
            status |= SPU_STATUS_ISOLATED_MASK;
    }

    *(uint32_t*)(shadow_addr + 0x30) = status;

    uint64_t someval = *(uint64_t*)(shadow_addr + 0xf10);
    someval |= r4_2;
    *(uint64_t*)(shadow_addr + 0xf10) = someval;
}

__attribute__((section("main6"))) uint64_t stage6_main(
    uint64_t in_r3, uint64_t in_r4, uint64_t in_r5, uint64_t in_r6, uint64_t in_r7, uint64_t in_r8, uint64_t in_r9, uint64_t in_r10
)
{
    sc_puts_init();
    is_emmc = FetchIsEmmc();
    check_pc(0, 1);

    if (in_r10 == 1)
        Stage6_IsoLoadRequest(in_r3);
    else if (in_r10 == 2)
        return Stage6_GetSpuStatus(in_r3);
    else if (in_r10 == 3)
        Stage6_RequestExitIsolation(in_r3);
    else if (in_r10 == 4)
        Stage6_UpdateSPUStatusAndTransitionNotifierInShadowRegArea(in_r3, in_r4);
    else
    {
        lv1_puts("stage6_main bad r10!\n");
        dead_beep();
    }

    return 0;
}

__attribute__((noreturn, section("entry6"))) void stage6_entry()
{
    // push stack
    ASM("addi 1, 1, -512");

    // store all registers to stack
    ASMX("std 0, %0(1)" ::"i"(8 * 0) :);
    ASMX("std 1, %0(1)" ::"i"(8 * 1) :);
    ASMX("std 2, %0(1)" ::"i"(8 * 2) :);
    ASMX("std 3, %0(1)" ::"i"(8 * 3) :);
    ASMX("std 4, %0(1)" ::"i"(8 * 4) :);
    ASMX("std 5, %0(1)" ::"i"(8 * 5) :);
    ASMX("std 6, %0(1)" ::"i"(8 * 6) :);
    ASMX("std 7, %0(1)" ::"i"(8 * 7) :);
    ASMX("std 8, %0(1)" ::"i"(8 * 8) :);
    ASMX("std 9, %0(1)" ::"i"(8 * 9) :);
    ASMX("std 10, %0(1)" ::"i"(8 * 10) :);
    ASMX("std 11, %0(1)" ::"i"(8 * 11) :);
    ASMX("std 12, %0(1)" ::"i"(8 * 12) :);
    ASMX("std 13, %0(1)" ::"i"(8 * 13) :);
    ASMX("std 14, %0(1)" ::"i"(8 * 14) :);
    ASMX("std 15, %0(1)" ::"i"(8 * 15) :);
    ASMX("std 16, %0(1)" ::"i"(8 * 16) :);
    ASMX("std 17, %0(1)" ::"i"(8 * 17) :);
    ASMX("std 18, %0(1)" ::"i"(8 * 18) :);
    ASMX("std 19, %0(1)" ::"i"(8 * 19) :);
    ASMX("std 20, %0(1)" ::"i"(8 * 20) :);
    ASMX("std 21, %0(1)" ::"i"(8 * 21) :);
    ASMX("std 22, %0(1)" ::"i"(8 * 22) :);
    ASMX("std 23, %0(1)" ::"i"(8 * 23) :);
    ASMX("std 24, %0(1)" ::"i"(8 * 24) :);
    ASMX("std 25, %0(1)" ::"i"(8 * 25) :);
    ASMX("std 26, %0(1)" ::"i"(8 * 26) :);
    ASMX("std 27, %0(1)" ::"i"(8 * 27) :);
    ASMX("std 28, %0(1)" ::"i"(8 * 28) :);
    ASMX("std 29, %0(1)" ::"i"(8 * 29) :);
    ASMX("std 30, %0(1)" ::"i"(8 * 30) :);
    ASMX("std 31, %0(1)" ::"i"(8 * 31) :);

#if 1

    register uint64_t r8 asm("r8");

    // push stack
    ASM("addi 1, 1, -64");

    // store original rtoc to stack
    ASM("std 2, 0(1)");

    // store original lr to stack
    ASMX("mflr %0" : "=r"(r8)::);
    ASMX("std %0, 8(1)" ::"r"(r8) :);

    // set stage_entry_ra
    ASM("bl 4");
    ASMX("mflr %0" : "=r"(stage_entry_ra)::);
    stage_entry_ra -= (4 * 38);

    // set lv1_rtoc
    ASMX("mr %0, 2" : "=r"(lv1_rtoc)::);

    // set is_lv1 to 0x9669
    is_lv1 = 0x9669;

    // set stage_zero to 0
    stage_zero = 0;

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x200; // .toc
    stage_rtoc += 0x8000;

    // set r2 to stage_rtoc
    ASMX("mr 2, %0" ::"r"(stage_rtoc) :);

    // set lv1_sp
    ASMX("mr %0, 1" : "=r"(lv1_sp)::);

    // set stage_sp to 0xE000000
    //stage_sp = 0xE000000;

    // set r1 to stage_sp
    //ASM("mr 1, %0" ::"r"(stage_sp) :);

    // push stack
    ASM("addi 1, 1, -128");

    // call stage6_main
    ASM("bl stage6_main");

    // pop stack
    ASM("addi 1, 1, 128");

    // set r1 to lv1_sp
    ASMX("mr 1, %0" ::"r"(lv1_sp) :);

    // restore original lr from stack
    ASMX("ld %0, 8(1)" : "=r"(r8)::);
    ASMX("mtlr %0" ::"r"(r8):);

    // restore original rtoc from stack
    ASMX("ld %0, 0(1)" : "=r"(r8)::);
    ASMX("mr 2, %0" ::"r"(r8):);

    // pop stack
    ASM("addi 1, 1, 64");

#endif

    // restore all registers from stack
    ASMX("ld 0, %0(1)" ::"i"(8 * 0) :);
    ASMX("ld 1, %0(1)" ::"i"(8 * 1) :);
    ASMX("ld 2, %0(1)" ::"i"(8 * 2) :);
    //ASMX("ld 3, %0(1)" ::"i"(8 * 3) :);
    ASMX("ld 4, %0(1)" ::"i"(8 * 4) :);
    ASMX("ld 5, %0(1)" ::"i"(8 * 5) :);
    ASMX("ld 6, %0(1)" ::"i"(8 * 6) :);
    ASMX("ld 7, %0(1)" ::"i"(8 * 7) :);
    ASMX("ld 8, %0(1)" ::"i"(8 * 8) :);
    ASMX("ld 9, %0(1)" ::"i"(8 * 9) :);
    ASMX("ld 10, %0(1)" ::"i"(8 * 10) :);
    ASMX("ld 11, %0(1)" ::"i"(8 * 11) :);
    ASMX("ld 12, %0(1)" ::"i"(8 * 12) :);
    ASMX("ld 13, %0(1)" ::"i"(8 * 13) :);
    ASMX("ld 14, %0(1)" ::"i"(8 * 14) :);
    ASMX("ld 15, %0(1)" ::"i"(8 * 15) :);
    ASMX("ld 16, %0(1)" ::"i"(8 * 16) :);
    ASMX("ld 17, %0(1)" ::"i"(8 * 17) :);
    ASMX("ld 18, %0(1)" ::"i"(8 * 18) :);
    ASMX("ld 19, %0(1)" ::"i"(8 * 19) :);
    ASMX("ld 20, %0(1)" ::"i"(8 * 20) :);
    ASMX("ld 21, %0(1)" ::"i"(8 * 21) :);
    ASMX("ld 22, %0(1)" ::"i"(8 * 22) :);
    ASMX("ld 23, %0(1)" ::"i"(8 * 23) :);
    ASMX("ld 24, %0(1)" ::"i"(8 * 24) :);
    ASMX("ld 25, %0(1)" ::"i"(8 * 25) :);
    ASMX("ld 26, %0(1)" ::"i"(8 * 26) :);
    ASMX("ld 27, %0(1)" ::"i"(8 * 27) :);
    ASMX("ld 28, %0(1)" ::"i"(8 * 28) :);
    ASMX("ld 29, %0(1)" ::"i"(8 * 29) :);
    ASMX("ld 30, %0(1)" ::"i"(8 * 30) :);
    ASMX("ld 31, %0(1)" ::"i"(8 * 31) :);

    // pop stack
    ASM("addi 1, 1, 512");

    //
    ASM("blr");

    __builtin_unreachable();
}