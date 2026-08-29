FUNC_DEF void ApplyLv1Diff(uint64_t lv1DiffFileAddress, uint8_t verifyOrig)
{
    puts("ApplyLv1Diff()\n");

    uint64_t curAddress = lv1DiffFileAddress;

    uint32_t diffCount = *((const uint32_t *)curAddress);
    curAddress += 4;

    puts("diffCount = ");
    print_decimal(diffCount);
    puts("\n");

    for (uint32_t i = 0; i < diffCount; ++i)
    {
        uint32_t addr = *((const uint32_t *)curAddress);
        curAddress += 4;

        uint32_t value = *((const uint32_t *)curAddress);
        curAddress += 4;

        uint8_t origVal = (uint8_t)(value >> 8);
        uint8_t newVal = (uint8_t)(value & 0xFF);

#if 0
        puts("addr = ");
        print_hex(addr);

        puts(", origVal = ");
        print_hex(origVal);

        puts(", newVal = ");
        print_hex(newVal);

        puts("\n");
#endif

        if (verifyOrig)
        {
            uint8_t curVal = *((const uint8_t *)(uint64_t)addr);

            if (curVal != origVal)
            {
                puts("verifyOrig failed at addr = ");
                print_hex(addr);

                puts(", curVal = ");
                print_hex(curVal);

                puts(", origVal = ");
                print_hex(origVal);
                puts("\n");

                dead_beep();
            }
        }

        *((uint8_t *)(uint64_t)addr) = newVal;
    }

    puts("ApplyLv1Diff() done.\n");
}

FUNC_DEF void Stage2()
{
    puts("BadWDSD Stage2 by Kafuu(aomsin2526)\n");

    puts("(Build Date: ");
    puts(__DATE__);
    puts(" ");
    puts(__TIME__);
    puts(")\n");

    print_pc();

    uint8_t os_bank_indicator = get_os_bank_indicator();

    uint8_t isqCFW = CoreOS2_Bank_IsqCFW(os_bank_indicator);
    uint8_t isqCFW_jig = CoreOS2_Bank_IsqCFW_jig(os_bank_indicator);

    uint8_t qcfw_lite_flag = get_qcfw_lite_flag();

    uint16_t fwVersion = CoreOS2_Bank_GetFWVersion(os_bank_indicator);

    {
        {
            static const uint64_t lv1DiffFileBuf_MaxSize = (64 * 1024);
            __attribute__((aligned(8))) uint8_t lv1DiffFileBuf[lv1DiffFileBuf_MaxSize];

            uint32_t lv1DiffFileFlashOffset = 0;
            uint64_t lv1DiffFileAddress = (uint64_t)lv1DiffFileBuf;
            uint32_t lv1DiffFileSize = 0;

            if (!(isqCFW || isqCFW_jig) && (qcfw_lite_flag == 0x1))
            {
                if (fwVersion == 492)
                {
                    puts("Searching for qcfwlite492cex_lv1.diff...\n");
                    CoreOS2_FindFileEntry_Aux("qcfwlite492cex_lv1.diff", &lv1DiffFileFlashOffset, &lv1DiffFileSize);
                }
                else if (fwVersion == 493)
                {
                    puts("Searching for qcfwlite493cex_lv1.diff...\n");
                    CoreOS2_FindFileEntry_Aux("qcfwlite493cex_lv1.diff", &lv1DiffFileFlashOffset, &lv1DiffFileSize);
                }
                else
                {
                    puts("Current firmware doesn't support qCFW lite!\n");
                    dead_beep();
                }

                if (lv1DiffFileFlashOffset == 0)
                {
                    puts("File not found!\n");
                    dead_beep();
                }
            }
            else
            {
                puts("Searching for lv1.diff...\n");
                CoreOS2_FindFileEntry_Bank(os_bank_indicator, "lv1.diff", &lv1DiffFileFlashOffset, &lv1DiffFileSize);
            }

            if (lv1DiffFileFlashOffset != 0)
            {
                puts("lv1DiffFileFlashOffset = ");
                print_hex(lv1DiffFileFlashOffset);

                puts(", lv1DiffFileAddress = ");
                print_hex(lv1DiffFileAddress);

                puts(", lv1DiffFileSize = ");
                print_decimal(lv1DiffFileSize);

                puts("\n");

                if (lv1DiffFileSize > lv1DiffFileBuf_MaxSize)
                {
                    puts("File too big!\n");
                    dead_beep();
                }

                FlashRead(lv1DiffFileFlashOffset, (void*)lv1DiffFileAddress, lv1DiffFileSize);

                ApplyLv1Diff(lv1DiffFileAddress, 1);
            }
            else
                puts("File not found!\n");
        }

        {
            struct Stagex_spu_job_stage2_context_s job_context;
            job_context.is_qcfw_jig = 0;

            if (isqCFW_jig)
            {
                job_context.is_qcfw_jig = 1;
            }
            else
            {
                job_context.patch_aim = 0;
                job_context.patch_inspect_package_tophalf = 0;

                if (isqCFW)
                {
                    if (CoreOS2_FindFileEntry_Bank(os_bank_indicator, "lv2Rkernel.self", NULL, NULL))
                    {
                        uint8_t tid = read_targetid();

                        if (tid != 0x82)
                        {
                            job_context.patch_aim = 1;
                            job_context.patch_inspect_package_tophalf = 1;
                        }
                    }
                }

                {
                    uint8_t fsm_counter = sc_read_fsm_counter();
                    uint8_t fsm_toggle_flag = sc_read_fsm_toggle_flag();

                    puts("fsm_counter = ");
                    print_decimal(fsm_counter);
                    puts("\n");

                    puts("fsm_toggle_flag = ");
                    print_hex(fsm_toggle_flag);
                    puts("\n");

                    job_context.fsm_toggle = 0xff; // do nothing

                    if (fsm_toggle_flag == 0xfe) // always do nothing
                    {
                        puts("ALWAYS DO NOTHING\n");
                        sc_write_fsm_counter(0);
                    }
                    else if (fsm_toggle_flag == 0x1) // enter
                    {
                        real_puts("ENTER FSM\n");

                        job_context.fsm_toggle = 0x1;

                        sc_write_fsm_counter(0);
                        sc_write_fsm_toggle_flag(0xff);
                    }
                    else if ((fsm_toggle_flag == 0x0) || (fsm_counter >= 5)) // exit
                    {
                        if (fsm_toggle_flag == 0x0)
                            real_puts("EXIT FSM\n");
                        else
                            puts("EXIT FSM\n");

                        job_context.fsm_toggle = 0x0;

                        sc_write_fsm_counter(0);
                        sc_write_fsm_toggle_flag(0xff);
                    }
                    else
                    {
                        puts("DO NOTHING (INCREMENT COUNTER)\n");

                        sc_write_fsm_counter(fsm_counter + 1);
                        sc_write_fsm_toggle_flag(0xff);
                    }
                }
            }

            {
                static const uint64_t stagexSpuElf_MaxSize = stagex_aux_max_size;
                __attribute__((aligned(8))) uint8_t stagexSpuElf[stagexSpuElf_MaxSize];

                SpuAux_CopyElfToMem(stagexSpuElf, stagexSpuElf_MaxSize);

                uint64_t spu_id = calc_myspu_id();
                uint64_t spu_old_mfc_sr1 = SpuAux_Init(spu_id, stagexSpuElf);
                spu_stage2(spu_id, &job_context);
                SpuAux_Uninit(spu_id, spu_old_mfc_sr1);
            }
        }

        if (isqCFW || (qcfw_lite_flag == 0x1))
        {
            struct Stagex_Context_s* ctx = GetStagexContext_Unchecked();

            //

            ctx->cached_is_emmc = is_emmc;

            puts("cached_is_emmc = ");
            print_decimal(ctx->cached_is_emmc);
            puts("\n");

            //

            ctx->cached_os_bank_indicator = os_bank_indicator;

            puts("cached_os_bank_indicator = ");
            print_hex(ctx->cached_os_bank_indicator);
            puts("\n");

            //

            ctx->cached_qcfw_lite_flag = qcfw_lite_flag;

            puts("cached_qcfw_lite_flag = ");
            print_hex(ctx->cached_qcfw_lite_flag);
            puts("\n");

            //

            ctx->stage3_alreadyDone = 0;

            ctx->stage6_isAppldr = 0;
            ctx->stage6_isLv2ldr = 0;
            ctx->stage6_isLv2ldr_rvk = 0;

            ctx->stage6_spu_id = 0xff; // unknown

            //

            static const uint8_t lv1ldr_idx = 0;
            //static const uint8_t metldr_idx = 1;
            static const uint8_t lv2ldr_idx = 2;
            //static const uint8_t isoldr_idx = 3;
            static const uint8_t appldr_idx = 4;
            //static const uint8_t eid0_idx = 5;
            //static const uint8_t is_qa_flag_idx = 6;
            //static const uint8_t qa_token_bin_idx = 7;
            //static const uint8_t trace_level_idx = 8;

            // heap:
            // ptr: 0x10190
            // size: 0x66000

            uint64_t heapAddr = 0x10190;
            uint64_t heapSize = 0x66000;

            uint64_t tmpHeapAddr = 0x2000000;

            static const uint64_t comp_count = 9;
            struct comp_entry_s* comps = (struct comp_entry_s*)0x10100;

            struct comp_entry_s tmp_comps[comp_count];

            for (uint64_t i = 0; i < comp_count; ++i)
            {
                if (i == lv1ldr_idx)
                    continue;

                struct comp_entry_s* dest = &tmp_comps[i];
                const struct comp_entry_s* src = &comps[i];

                dest->ptr = src->ptr;
                dest->ptr += tmpHeapAddr;
                dest->ptr -= heapAddr;

                dest->size = src->size;

                memcpy(dest->ptr, src->ptr, dest->size);
            }

            struct SimpleHeap_s heap;
            SimpleHeap_Init(&heap, (void*)heapAddr, heapSize);

            {
                static const uint64_t stagex_size = stagex_max_size;
                uint64_t old_stagex_addr = 0x1010000;

                ctx->cached_Stagex = SimpleHeap_Alloc(&heap, stagex_size, 32);
                uint64_t new_stagex_addr = (uint64_t)ctx->cached_Stagex;

                Stagex_Relocate((const void*)old_stagex_addr, old_stagex_addr, new_stagex_addr);

                puts("cached_Stagex = ");
                print_hex((uint64_t)ctx->cached_Stagex);
                puts("\n");

                {
                    *((uint64_t*)0x120) = (new_stagex_addr + 0x200);
                    *((uint64_t*)0x210) = (new_stagex_addr + 0x400);
                    *((uint64_t*)0x220) = (new_stagex_addr + 0x700);
                }
            }

#if SC_PUTS_BUFFER_ENABLED
            ctx->sc_puts_buflen = 0;
            ctx->sc_puts_buf = (char*)SimpleHeap_Alloc(&heap, 512, 8);

            puts("sc_puts_buf = ");
            print_hex((uint64_t)ctx->sc_puts_buf);
            puts("\n");
#endif

            if (CoreOS2_FindFileEntry_Aux("Stagex_spu.elf", &ctx->cached_StagexSpuElf_FileFlashOffset, &ctx->cached_StagexSpuElf_FileSize))
            {
                puts("cached_StagexSpuElf_FileFlashOffset = ");
                print_hex(ctx->cached_StagexSpuElf_FileFlashOffset);
                puts("\n");

                puts("cached_StagexSpuElf_FileSize = ");
                print_decimal(ctx->cached_StagexSpuElf_FileSize);
                puts("\n");
            }
            else
            {
                puts("Stagex_spu.elf not found!\n");
                dead_beep();
            }

            {
                ctx->cached_mymetldrElf = NULL;

                {
                    uint32_t fileFlashOffset = 0;
                    uint32_t fileSize = 0;

                    if (CoreOS2_FindFileEntry_Bank(os_bank_indicator, "mymetldr.elf", &fileFlashOffset, &fileSize))
                    {
                        ctx->cached_mymetldrElf = SimpleHeap_Alloc(&heap, fileSize, 32);
                        FlashRead(fileFlashOffset, ctx->cached_mymetldrElf, fileSize);
                    }
                }

                puts("cached_mymetldrElf = ");
                print_hex((uint64_t)ctx->cached_mymetldrElf);
                puts("\n");
            }

            ctx->has_mylv2ldr = 0;
            ctx->has_myappldr = 0;

            if (CoreOS2_FindFileEntry_Bank(os_bank_indicator, "mylv2ldr.elf", &ctx->cached_mylv2ldrElf_FileFlashOffset, &ctx->cached_mylv2ldrElf_FileSize))
            {
                ctx->has_mylv2ldr = 1;

                puts("cached_mylv2ldrElf_FileFlashOffset = ");
                print_hex(ctx->cached_mylv2ldrElf_FileFlashOffset);
                puts("\n");

                puts("cached_mylv2ldrElf_FileSize = ");
                print_decimal(ctx->cached_mylv2ldrElf_FileSize);
                puts("\n");
            }

            if (CoreOS2_FindFileEntry_Bank(os_bank_indicator, "myappldr.elf", &ctx->cached_myappldrElf_FileFlashOffset, &ctx->cached_myappldrElf_FileSize))
            {
                ctx->has_myappldr = 1;

                puts("cached_myappldrElf_FileFlashOffset = ");
                print_hex(ctx->cached_myappldrElf_FileFlashOffset);
                puts("\n");

                puts("cached_myappldrElf_FileSize = ");
                print_decimal(ctx->cached_myappldrElf_FileSize);
                puts("\n");
            }

            if (CoreOS2_FindFileEntry_Bank(os_bank_indicator, "lv0", &ctx->cached_lv0_FileFlashOffset, &ctx->cached_lv0_FileSize))
            {
                puts("cached_lv0_FileFlashOffset = ");
                print_hex(ctx->cached_lv0_FileFlashOffset);
                puts("\n");

                puts("cached_lv0_FileSize = ");
                print_decimal(ctx->cached_lv0_FileSize);
                puts("\n");
            }
            else
            {
                puts("lv0 not found!\n");
                dead_beep();
            }

            {
                // cache metadata from (lv2ldrSelf + 0x210), size = 0x370
                ctx->cached_lv2ldr_meta = SimpleHeap_Alloc(&heap, 0x370, 8);

                memcpy(ctx->cached_lv2ldr_meta, (const void*)(tmp_comps[lv2ldr_idx].ptr + 0x210), 0x370);

                puts("cached_lv2ldr_meta = ");
                print_hex((uint64_t)ctx->cached_lv2ldr_meta);
                puts("\n");
            }

            {
                // cache metadata from (appldrSelf + 0x210), size = 0x370
                ctx->cached_appldr_meta = SimpleHeap_Alloc(&heap, 0x370, 8);

                memcpy(ctx->cached_appldr_meta, (const void*)(tmp_comps[appldr_idx].ptr + 0x210), 0x370);

                puts("cached_appldr_meta = ");
                print_hex((uint64_t)ctx->cached_appldr_meta);
                puts("\n");
            }

            for (uint64_t i = 0; i < comp_count; ++i)
            {
                if (i == lv1ldr_idx)
                    continue;
                if (i == appldr_idx)
                    continue;

                uint8_t isldr = (i <= 4) ? 1 : 0;

                struct comp_entry_s* dest = &comps[i];
                const struct comp_entry_s* src = &tmp_comps[i];

                dest->size = src->size;

                uint64_t allocSize = dest->size;

                if (i == lv2ldr_idx)
                {
                    {
                        const struct comp_entry_s* appldr_comp = &tmp_comps[appldr_idx];

                        if (allocSize < appldr_comp->size)
                            allocSize = appldr_comp->size;
                    }

                    if (ctx->has_mylv2ldr)
                    {
                        if (allocSize < ctx->cached_mylv2ldrElf_FileSize)
                            allocSize = ctx->cached_mylv2ldrElf_FileSize;
                    }

                    if (ctx->has_myappldr)
                    {
                        if (allocSize < ctx->cached_myappldrElf_FileSize)
                            allocSize = ctx->cached_myappldrElf_FileSize;
                    }

                    ctx->cached_sharedLdr_Size = allocSize;

                    puts("cached_sharedLdr_Size = ");
                    print_decimal(ctx->cached_sharedLdr_Size);
                    puts("\n");
                }

                dest->ptr = SimpleHeap_Alloc(&heap, allocSize, isldr ? 0x80 : 0x10);

                if (i == lv2ldr_idx)
                {
                    ctx->cached_sharedLdr = dest->ptr;
                    ctx->cached_sharedLdr_CurKind = 1; // lv2ldr

                    puts("cached_sharedLdr = ");
                    print_hex((uint64_t)ctx->cached_sharedLdr);
                    puts("\n");
                }

                puts("comp ");
                print_decimal(i);
                puts(", ptr = ");
                print_hex((uint64_t)dest->ptr);
                puts(", size = ");
                print_decimal(dest->size);
                puts(", allocSize = ");
                print_decimal(allocSize);
                puts("\n");

                memcpy(dest->ptr, src->ptr, dest->size);
            }

            // point it to some valid headers

            {
                struct comp_entry_s* dest = &comps[appldr_idx];
                dest->ptr = (uint8_t*)ctx->cached_sharedLdr;
            }

            //

            ctx->cached_fwVersion = fwVersion;

            puts("cached_fwVersion = ");
            print_decimal(ctx->cached_fwVersion);
            puts("\n");

            //

            ctx->magic = 0xca8fe91729035026;

            //
        }
    }

    if (isqCFW || isqCFW_jig)
        sc_led_static_yellow();
    else
        sc_led_static_green();

    puts("Booting lv1...\n");

    ASM("li 3, 0x100");
    ASM("mtctr 3");
    ASM("bctr");
}

#if HDDKEYDUMPER_ENABLED
#include "Stage2_HDDKeyDumper.c"
#endif

__attribute__((section("main2"))) void stage2_main(uint64_t in_r3)
{
    is_emmc = FetchIsEmmc();

    real_sc_puts_init();

#if HDDKEYDUMPER_ENABLED
    if (sc_read_hdd_key_dumper_flag() == 0x2)
        Stage2_HDDKeyDumper(in_r3);
    else
#endif
        Stage2();

    dead_beep();
}

__attribute__((noreturn, section("entry2"))) void stage2_entry()
{
    // set stage_entry_ra
    ASM("bl 4");
    ASM("mflr %0" : "=r"(stage_entry_ra)::);
    stage_entry_ra -= 4;

    // set is_lv1 to 0
    is_lv1 = 0;

    // set stage_zero to 0
    stage_zero = 0;

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x800; // .toc
    stage_rtoc += 0x8000;

    // set r2 to stage_rtoc
    ASM("mr 2, %0" ::"r"(stage_rtoc) :);

    // set stage_sp to 0xDFFFF00
    stage_sp = 0xDFFFF00;

    // set r1 to stage_sp
    ASM("mr 1, %0" ::"r"(stage_sp) :);

    // sync
    ASM("sync");

    // push stack
    ASM("addi 1, 1, -128");

    // jump to stage_main
    ASM("b stage2_main");

    __builtin_unreachable();
}