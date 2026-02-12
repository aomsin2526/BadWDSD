struct __attribute__((aligned(8))) mymetldr_context_s
{
    uint64_t myldrElfAddress;
};

FUNC_DEF void Stage6_IsoLoadRequest(uint64_t spu_id)
{
    //lv1_puts("Stage6_IsoLoadRequest()\n");
    
    //lv1_puts("spu_id = ");
    //lv1_print_decimal(spu_id);
    //lv1_puts("\n");

    uint32_t status;

    struct Stagex_Context_s* ctx = GetStagexContext();

    //

    if (ctx->stage6_spu_id == 0xff)
        ctx->stage6_spu_id = (uint8_t)spu_id;

    //

    uint64_t myappldrElfAddress = ctx->cached_myappldrElfAddress;
    uint64_t mylv2ldrElfAddress = ctx->cached_mylv2ldrElfAddress;

    uint8_t ok = 0;
    uint64_t myldrElfAddress = 0;

    if (spu_id == ctx->stage6_spu_id)
    {
        if (ctx->stage6_isAppldr && (myappldrElfAddress != 0))
        {
            myldrElfAddress = myappldrElfAddress;
            ok = 1;
        }

        if (ctx->stage6_isLv2ldr && (mylv2ldrElfAddress != 0))
        {
            myldrElfAddress = mylv2ldrElfAddress;
            ok = 1;
        }

        ctx->stage6_isAppldr = 0;
        ctx->stage6_isLv2ldr = 0;
    }

    if (!ok)
    {
        SPU_PS_Write32(spu_id, 0x0401C, 0x3);
        return;
    }

    //

    {
        struct ElfHeader32_s *elfHdr = (struct ElfHeader32_s *)myldrElfAddress;

        if (*((uint32_t *)elfHdr->e_ident) != 0x7F454C46)
        {
            SPU_PS_Write32(spu_id, 0x0401C, 0x3);
            return;
        }
    }

    //

    uint64_t mymetldrElfAddress = ctx->cached_mymetldrElfAddress;

    if (mymetldrElfAddress == 0)
    {
        lv1_puts("mymetldr.elf not found!\n");
        dead_beep();
    }

    status = SPU_PS_Read32(spu_id, 0x04024);
    if ((status & SPU_STATUS_RUN_MASK) != 0)
    {
        // stop request
        SPU_PS_Write32(spu_id, 0x0401C, 0x0);

        while ((status & SPU_STATUS_RUN_MASK) != 0)
        {
            status = SPU_PS_Read32(spu_id, 0x04024);
        }
    }

    status = SPU_PS_Read32(spu_id, 0x04024);
    if ((status & SPU_STATUS_ISOLATED_MASK) != 0)
    {
        // iso exit request
        SPU_PS_Write32(spu_id, 0x0401C, 0x2);

        while ((status & SPU_STATUS_ISOLATED_MASK) != 0)
        {
            status = SPU_PS_Read32(spu_id, 0x04024);
        }
    }

    //lv1_puts("Loading mymetldr.elf...\n");
    LoadElfSpu(mymetldrElfAddress, spu_id, 1);
    
    {
        struct mymetldr_context_s mctx;
        mctx.myldrElfAddress = myldrElfAddress;

        memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &mctx, sizeof(mctx));
    }

    {
        // idps
        //const uint64_t* idps = (const uint64_t*)0x2401F02F070;

        //SPU_LS_Write64(spu_id, 0x39050, idps[0]);
        //SPU_LS_Write64(spu_id, 0x39058, idps[1]);

        SPU_LS_Write64(spu_id, 0x39050, 0x0000000100820001);
        SPU_LS_Write64(spu_id, 0x39058, 0x040034D1807DED30);

        // tid
        //const uint8_t* tid = (const uint8_t*)0x2401F02F075;
        //SPU_LS_Write64(spu_id, 0x39060, *tid);

        SPU_LS_Write64(spu_id, 0x39060, 0x82);
    }

    eieio();
    SPU_PS_Write32(spu_id, 0x0401C, 0x1);
}

FUNC_DEF uint32_t Stage6_GetSpuStatus(uint64_t spu_id)
{
    //lv1_puts("Stage6_GetSpuStatus()\n");
    
    //lv1_puts("spu_id = ");
    //lv1_print_decimal(spu_id);
    //lv1_puts("\n");

    uint32_t status = SPU_PS_Read32(spu_id, 0x04024);

    if (((status & SPU_STATUS_ISOLATED_MASK) == 0) && (SPU_LS_Read64(spu_id, 0x39100) == 0x123456789))
        status |= SPU_STATUS_ISOLATED_MASK;

#if 0

    lv1_puts("status = ");
    lv1_print_hex(status);
    lv1_puts("\n");

    uint32_t npc = SPU_PS_Read32(spu_id, 0x04034);
    lv1_puts("npc = ");
    lv1_print_hex(npc);
    lv1_puts("\n");

#if 1

    struct Stagex_spu_DMACmd_s dmaCmd;
    memcpy(&dmaCmd, (const void*)SPU_CalcMMIOAddress_LS(spu_id, 0x10), sizeof(dmaCmd));

    lv1_puts("dmaCmd.ls = ");
    lv1_print_hex(dmaCmd.ls);
    lv1_puts("\n");

    lv1_puts("dmaCmd.ea = ");
    lv1_print_hex(dmaCmd.ea);
    lv1_puts("\n");

    lv1_puts("dmaCmd.size = ");
    lv1_print_decimal(dmaCmd.size);
    lv1_puts("\n");

    lv1_puts("dmaCmd.cmd = ");
    lv1_print_hex(dmaCmd.cmd);
    lv1_puts("\n");

#endif

#endif

    return status;
}

FUNC_DEF void Stage6_RequestExitIsolation(uint64_t spu_id)
{
    //lv1_puts("Stage6_RequestExitIsolation()\n");

    {
        uint32_t status = SPU_PS_Read32(spu_id, 0x04024);

        if (((status & SPU_STATUS_ISOLATED_MASK) == 0) && (SPU_LS_Read64(spu_id, 0x39100) == 0x123456789))
        {
            //lv1_puts("xxx\n");
            SPU_LS_Write64(spu_id, 0x39100, 0x0);
        }
    }

    SPU_PS_Write32(spu_id, 0x0401C, 0x2);
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
        {
            //lv1_puts("yyy\n");
            status |= SPU_STATUS_ISOLATED_MASK;
        }
    }

    //lv1_puts("status = ");
    //lv1_print_hex(status);
    //lv1_puts("\n");

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
        dead();
    }

    return 0;
}

__attribute__((noreturn, section("entry6"))) void stage6_entry()
{
    // push stack
    asm volatile("addi 1, 1, -512");

    // store all registers to stack
    asm volatile("std 0, %0(1)" ::"i"(8 * 0) :);
    asm volatile("std 1, %0(1)" ::"i"(8 * 1) :);
    asm volatile("std 2, %0(1)" ::"i"(8 * 2) :);
    asm volatile("std 3, %0(1)" ::"i"(8 * 3) :);
    asm volatile("std 4, %0(1)" ::"i"(8 * 4) :);
    asm volatile("std 5, %0(1)" ::"i"(8 * 5) :);
    asm volatile("std 6, %0(1)" ::"i"(8 * 6) :);
    asm volatile("std 7, %0(1)" ::"i"(8 * 7) :);
    asm volatile("std 8, %0(1)" ::"i"(8 * 8) :);
    asm volatile("std 9, %0(1)" ::"i"(8 * 9) :);
    asm volatile("std 10, %0(1)" ::"i"(8 * 10) :);
    asm volatile("std 11, %0(1)" ::"i"(8 * 11) :);
    asm volatile("std 12, %0(1)" ::"i"(8 * 12) :);
    asm volatile("std 13, %0(1)" ::"i"(8 * 13) :);
    asm volatile("std 14, %0(1)" ::"i"(8 * 14) :);
    asm volatile("std 15, %0(1)" ::"i"(8 * 15) :);
    asm volatile("std 16, %0(1)" ::"i"(8 * 16) :);
    asm volatile("std 17, %0(1)" ::"i"(8 * 17) :);
    asm volatile("std 18, %0(1)" ::"i"(8 * 18) :);
    asm volatile("std 19, %0(1)" ::"i"(8 * 19) :);
    asm volatile("std 20, %0(1)" ::"i"(8 * 20) :);
    asm volatile("std 21, %0(1)" ::"i"(8 * 21) :);
    asm volatile("std 22, %0(1)" ::"i"(8 * 22) :);
    asm volatile("std 23, %0(1)" ::"i"(8 * 23) :);
    asm volatile("std 24, %0(1)" ::"i"(8 * 24) :);
    asm volatile("std 25, %0(1)" ::"i"(8 * 25) :);
    asm volatile("std 26, %0(1)" ::"i"(8 * 26) :);
    asm volatile("std 27, %0(1)" ::"i"(8 * 27) :);
    asm volatile("std 28, %0(1)" ::"i"(8 * 28) :);
    asm volatile("std 29, %0(1)" ::"i"(8 * 29) :);
    asm volatile("std 30, %0(1)" ::"i"(8 * 30) :);
    asm volatile("std 31, %0(1)" ::"i"(8 * 31) :);

#if 1

    register uint64_t r8 asm("r8");

    // push stack
    asm volatile("addi 1, 1, -64");

    // store original rtoc to stack
    asm volatile("std 2, 0(1)");

    // store original lr to stack
    asm volatile("mflr %0" : "=r"(r8)::);
    asm volatile("std %0, 8(1)" ::"r"(r8) :);

    // set stage_entry_ra
    asm volatile("bl 4");
    asm volatile("mflr %0" : "=r"(stage_entry_ra)::);
    stage_entry_ra -= (4 * 38);

    // set lv1_rtoc
    asm volatile("mr %0, 2" : "=r"(lv1_rtoc)::);

    // set is_lv1 to 0x9669
    is_lv1 = 0x9669;

    // set stage_zero to 0
    stage_zero = 0;

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x200; // .toc
    stage_rtoc += 0x8000;

    // set r2 to stage_rtoc
    asm volatile("mr 2, %0" ::"r"(stage_rtoc) :);

    // set lv1_sp
    asm volatile("mr %0, 1" :"=r"(lv1_sp)::);

    // set stage_sp to 0xE000000
    //stage_sp = 0xE000000;

    // set r1 to stage_sp
    //asm volatile("mr 1, %0" ::"r"(stage_sp) :);

    // sync
    asm volatile("sync");

    // push stack
    asm volatile("addi 1, 1, -128");

    // jump to stage6_main
    asm volatile("bl stage6_main");

    // pop stack
    asm volatile("addi 1, 1, 128");

    // set r1 to lv1_sp
    asm volatile("mr 1, %0" ::"r"(lv1_sp) :);

    // restore original lr from stack
    asm volatile("ld %0, 8(1)" : "=r"(r8)::);
    asm volatile("mtlr %0" ::"r"(r8));

    // restore original rtoc from stack
    asm volatile("ld %0, 0(1)" : "=r"(r8)::);
    asm volatile("mr 2, %0" ::"r"(r8));

    // pop stack
    asm volatile("addi 1, 1, 64");

#endif

    // restore all registers from stack
    asm volatile("ld 0, %0(1)" ::"i"(8 * 0) :);
    asm volatile("ld 1, %0(1)" ::"i"(8 * 1) :);
    asm volatile("ld 2, %0(1)" ::"i"(8 * 2) :);
    //asm volatile("ld 3, %0(1)" ::"i"(8 * 3) :);
    asm volatile("ld 4, %0(1)" ::"i"(8 * 4) :);
    asm volatile("ld 5, %0(1)" ::"i"(8 * 5) :);
    asm volatile("ld 6, %0(1)" ::"i"(8 * 6) :);
    asm volatile("ld 7, %0(1)" ::"i"(8 * 7) :);
    asm volatile("ld 8, %0(1)" ::"i"(8 * 8) :);
    asm volatile("ld 9, %0(1)" ::"i"(8 * 9) :);
    asm volatile("ld 10, %0(1)" ::"i"(8 * 10) :);
    asm volatile("ld 11, %0(1)" ::"i"(8 * 11) :);
    asm volatile("ld 12, %0(1)" ::"i"(8 * 12) :);
    asm volatile("ld 13, %0(1)" ::"i"(8 * 13) :);
    asm volatile("ld 14, %0(1)" ::"i"(8 * 14) :);
    asm volatile("ld 15, %0(1)" ::"i"(8 * 15) :);
    asm volatile("ld 16, %0(1)" ::"i"(8 * 16) :);
    asm volatile("ld 17, %0(1)" ::"i"(8 * 17) :);
    asm volatile("ld 18, %0(1)" ::"i"(8 * 18) :);
    asm volatile("ld 19, %0(1)" ::"i"(8 * 19) :);
    asm volatile("ld 20, %0(1)" ::"i"(8 * 20) :);
    asm volatile("ld 21, %0(1)" ::"i"(8 * 21) :);
    asm volatile("ld 22, %0(1)" ::"i"(8 * 22) :);
    asm volatile("ld 23, %0(1)" ::"i"(8 * 23) :);
    asm volatile("ld 24, %0(1)" ::"i"(8 * 24) :);
    asm volatile("ld 25, %0(1)" ::"i"(8 * 25) :);
    asm volatile("ld 26, %0(1)" ::"i"(8 * 26) :);
    asm volatile("ld 27, %0(1)" ::"i"(8 * 27) :);
    asm volatile("ld 28, %0(1)" ::"i"(8 * 28) :);
    asm volatile("ld 29, %0(1)" ::"i"(8 * 29) :);
    asm volatile("ld 30, %0(1)" ::"i"(8 * 30) :);
    asm volatile("ld 31, %0(1)" ::"i"(8 * 31) :);

    // pop stack
    asm volatile("addi 1, 1, 512");

    // sync
    asm volatile("sync");

    //
    asm volatile("blr");

    __builtin_unreachable();
}