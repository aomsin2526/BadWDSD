// note: log should be disabled in normal use
// STAGE5_LOG_ENABLED

FUNC_DEF uint64_t SPU_CalcMMIOAddress(uint64_t spu_id, uint64_t offset)
{
    return 0x20000000000 + (0x80000 * spu_id) + offset;
}

FUNC_DEF uint64_t SPU_CalcMMIOAddress_LS(uint64_t spu_id, uint64_t offset)
{
    return SPU_CalcMMIOAddress(spu_id, offset);
}

FUNC_DEF uint64_t SPU_CalcMMIOAddress_PS(uint64_t spu_id, uint64_t offset)
{
    return SPU_CalcMMIOAddress(spu_id, offset) + 0x40000;
}

FUNC_DEF uint64_t SPU_CalcMMIOAddress_P2(uint64_t spu_id, uint64_t offset)
{
    return SPU_CalcMMIOAddress(spu_id, offset) + 0x60000;
}

FUNC_DEF uint64_t SPU_CalcMMIOAddress_P1(uint64_t spu_id, uint64_t offset)
{
    return 0x20000400000 + (0x2000 * spu_id) + offset;
}

FUNC_DEF uint64_t SPU_LS_Read64(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint64_t *)SPU_CalcMMIOAddress_LS(spu_id, offset));
}

FUNC_DEF void SPU_LS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_LS(spu_id, offset)) = value;
}

FUNC_DEF uint32_t SPU_LS_Read32(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint32_t *)SPU_CalcMMIOAddress_LS(spu_id, offset));
}

FUNC_DEF void SPU_LS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_LS(spu_id, offset)) = value;
}

FUNC_DEF uint64_t SPU_PS_Read64(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint64_t *)SPU_CalcMMIOAddress_PS(spu_id, offset));
}

FUNC_DEF void SPU_PS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_PS(spu_id, offset)) = value;
}

FUNC_DEF uint32_t SPU_PS_Read32(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint32_t *)SPU_CalcMMIOAddress_PS(spu_id, offset));
}

FUNC_DEF void SPU_PS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_PS(spu_id, offset)) = value;
}

FUNC_DEF uint64_t SPU_P2_Read64(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint64_t *)SPU_CalcMMIOAddress_P2(spu_id, offset));
}

FUNC_DEF void SPU_P2_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_P2(spu_id, offset)) = value;
}

FUNC_DEF uint32_t SPU_P2_Read32(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint32_t *)SPU_CalcMMIOAddress_P2(spu_id, offset));
}

FUNC_DEF void SPU_P2_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_P2(spu_id, offset)) = value;
}

FUNC_DEF uint64_t SPU_P1_Read64(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint64_t *)SPU_CalcMMIOAddress_P1(spu_id, offset));
}

FUNC_DEF void SPU_P1_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_P1(spu_id, offset)) = value;
}

FUNC_DEF uint32_t SPU_P1_Read32(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint32_t *)SPU_CalcMMIOAddress_P1(spu_id, offset));
}

FUNC_DEF void SPU_P1_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_P1(spu_id, offset)) = value;
}

FUNC_DEF void LoadElfSpu(uint64_t elfFileAddress, uint64_t spu_id)
{
    puts("LoadElfSpu()\n");

    struct ElfHeader32_s *elfHdr = (struct ElfHeader32_s *)elfFileAddress;

    if (*((uint32_t *)elfHdr->e_ident) != 0x7F454C46)
    {
        puts("LoadElf e_ident check failed!\n");
        dead();
    }

    puts("spu_id = ");
    print_decimal(spu_id);
    puts("\n");

    puts("e_entry = ");
    print_hex(elfHdr->e_entry);
    puts("\n");

    puts("e_phoff = ");
    print_hex(elfHdr->e_phoff);
    puts("\n");

    puts("e_phentsize = ");
    print_decimal(elfHdr->e_phentsize);
    puts("\n");

    puts("e_phnum = ");
    print_hex(elfHdr->e_phnum);
    puts("\n");

    uint64_t curPhdrAddress = (elfFileAddress + elfHdr->e_phoff);

    for (uint16_t i = 0; i < elfHdr->e_phnum; ++i)
    {
        struct ElfPhdr32_s *phdr = (struct ElfPhdr32_s *)curPhdrAddress;

        puts("p_offset = ");
        print_hex(phdr->p_offset);

        puts(", p_vaddr = ");
        print_hex(phdr->p_vaddr);

        puts(", p_paddr = ");
        print_hex(phdr->p_paddr);

        puts(", p_filesz = ");
        print_hex(phdr->p_filesz);

        puts(", p_memsz = ");
        print_hex(phdr->p_memsz);

        puts("\n");

        for (uint64_t i = 0; i < phdr->p_memsz; i += 8)
            SPU_LS_Write64(spu_id, (phdr->p_vaddr + i), 0);

        for (uint64_t i = 0; i < phdr->p_filesz; i += 8)
        {
            uint64_t v = *((uint64_t *)(elfFileAddress + phdr->p_offset + i));
            SPU_LS_Write64(spu_id, (phdr->p_vaddr + i), v);
        }

        curPhdrAddress += elfHdr->e_phentsize;
    }

    // SPU_NPC[0:29] = entry (LS)
    SPU_PS_Write32(spu_id, 0x04034, elfHdr->e_entry);

    eieio();

    puts("LoadElfSpu() done.\n");
}

#pragma GCC push_options
//#pragma GCC optimize("O0")

FUNC_DEF void Stage5_2()
{
    puts("Stage5_2()\n");

    uint64_t *stage5_loopCount = (uint64_t *)0x220;
    ++(*stage5_loopCount);

    if ((*stage5_loopCount) > 2)
        return;

    uint64_t *lv1_lv2AreaAddrPtr = (uint64_t *)0x370F20; // 0x8000000000000000
    uint64_t *lv1_lv2AreaSizePtr = (uint64_t *)0x370F28; // 0x352230

    uint64_t *lv1_lv2AreaHashPtr = (uint64_t *)0x370F30;

    uint64_t stage5_lv2AreaAddr;
    uint64_t stage5_lv2AreaSize = *lv1_lv2AreaSizePtr; // hash check size

    if (!FindLv2(&stage5_lv2AreaAddr))
    {
        puts("lv2 area not found!\n");
        return;
    }

    if ((*stage5_loopCount) == 2)
    {
        //ApplyLv2Diff(stage5_lv2AreaAddr, 1);
    }
    else
    {
        uint8_t* snapshot_ofw = (uint8_t*)0xA000000; // todo spu as storage?
        uint8_t* lv2AreaPtr = (uint8_t*)stage5_lv2AreaAddr;

        for (uint64_t offset = 0; offset < (*lv1_lv2AreaSizePtr); ++offset)
        {
            if ((offset % 0x10000) == 0)
            {
                print_hex(offset);
                puts("\n");
            }

            lv2AreaPtr[offset] = snapshot_ofw[offset];
        }
    }

    //*((uint64_t *)stage5_lv2AreaAddr + 0x150) = 0x9669;
    eieio();

    uint64_t *stage5_lv2HashCachePtr = (uint64_t *)0x218;
    uint64_t stage5_curLv2Hash = 0;

    for (uint64_t offset = 0; offset < stage5_lv2AreaSize; offset += 8)
        stage5_curLv2Hash += *((uint64_t *)(stage5_lv2AreaAddr + offset));

    puts("stage5_curLv2Hash = ");
    print_hex(stage5_curLv2Hash);
    puts("\n");

    if ((*stage5_lv2HashCachePtr) == stage5_curLv2Hash)
        return;

    *stage5_lv2HashCachePtr = stage5_curLv2Hash;

    puts("stage5_lv2AreaAddr = ");
    print_hex(stage5_lv2AreaAddr);

    puts(", stage5_lv2AreaSize = ");
    print_hex(stage5_lv2AreaSize);

    puts(", stage5_curLv2Hash = ");
    print_hex(stage5_curLv2Hash);

    puts("\n");

    puts("before_lv1_lv2AreaAddr = ");
    print_hex(*lv1_lv2AreaAddrPtr);

    puts(", before_lv1_lv2AreaSize = ");
    print_hex(*lv1_lv2AreaSizePtr);

    puts("\n");

    puts("before_lv1_lv2AreaHash[0] = ");
    print_hex(lv1_lv2AreaHashPtr[0]);
    puts("\n");

    puts("before_lv1_lv2AreaHash[1] = ");
    print_hex(lv1_lv2AreaHashPtr[1]);
    puts("\n");

    puts("before_lv1_lv2AreaHash[2] = ");
    print_hex(lv1_lv2AreaHashPtr[2]);
    puts("\n");

    {
        uint8_t os_bank_indicator = sc_read_os_bank_indicator();

        puts("os_bank_indicator = ");
        print_hex(os_bank_indicator);
        puts("\n");

        if (os_bank_indicator == 0xff)
            puts("Will use ros0\n");
        else
            puts("Will use ros1\n");

        uint64_t coreOSStartAddress = (os_bank_indicator == 0xff) ? 0x2401F0C0000 : 0x2401F7C0000;

        puts("Searching for lv2hashgen.elf...\n");

        uint64_t lv2HashGenFileAddress;
        uint64_t lv2HashGenFileSize;

        if (CoreOS_FindFileEntry(coreOSStartAddress, "lv2hashgen.elf", &lv2HashGenFileAddress, &lv2HashGenFileSize))
        {
            puts("lv2HashGenFileAddress = ");
            print_hex(lv2HashGenFileAddress);

            puts(", lv2HashGenFileSize = ");
            print_decimal(lv2HashGenFileSize);

            puts("\n");

            puts("Searching for free spu...\n");

            uint8_t found = 0;

            for (uint64_t spu_id = 0; spu_id < 8; ++spu_id)
            {
                if (spu_id == 3)
                    continue;

                uint32_t status = SPU_PS_Read32(spu_id, 0x04024);

                static const uint32_t SPU_STATUS_RUN_MASK = (1 << 0);
                static const uint32_t SPU_STATUS_ISOLATED_MASK = (1 << 7);

                if ((status & SPU_STATUS_ISOLATED_MASK) != 0)
                    continue;

                if ((status & SPU_STATUS_RUN_MASK) != 0)
                    continue;

                found = 1;

                puts("Found!\n");

                puts("spu_id = ");
                print_decimal(spu_id);
                puts("\n");

                // Sc_Rx: eid0 = 0x100830006
                // Sc_Rx: buffer_args_effective_addr = 0x8000000010086780
                // Sc_Rx: lv2prot0 = 0x8000000000000000
                // Sc_Rx: lv2prot1 = 0x352230
                // Sc_Rx: lv2hash0 = 0xf13f8865d14703f4
                // Sc_Rx: lv2hash1 = 0x749c5fd9a5fcc6b7
                // Sc_Rx: lv2hash2 = 0x2f5b40a000000000

                // Sc_Rx: 0x39010 = 0xf13f8865d14703f4
                // Sc_Rx: 0x39018 = 0x749c5fd9a5fcc6b7
                // Sc_Rx: 0x39020 = 0x2f5b40a000000000

                // 0x370F20
                // 80 00 00 00 00 00 00 00 00 00 00 00 00 35 22 30 F1 3F 88 65 D1 47 03 F4 74 9C 5F D9 A5 FC C6 B7 2F 5B 40 A0 00 00 00 00 00 00 00 00 00 00 00 00

                // relocation off

                uint64_t old_mfc_sr1 = SPU_P1_Read64(spu_id, 0x0);
                SPU_P1_Write64(spu_id, 0x0, (old_mfc_sr1 & 0xFFFFFFFFFFFFFFEF));

                //

                LoadElfSpu(lv2HashGenFileAddress, spu_id);
                eieio();

                //
                SPU_LS_Write64(spu_id, 0x3A0F0, stage5_lv2AreaAddr);
                SPU_LS_Write64(spu_id, 0x3A0F8, stage5_lv2AreaSize);

                eieio();

                // SPU_RUNCNTL = 0x1
                SPU_PS_Write32(spu_id, 0x0401C, 0x1);
                eieio();

                puts("Waiting for spu start...\n");

                // while ((status & SPU_STATUS_RUN_MASK) == 0)
                while ((status & 0xFF0000) != 0x690000) // 0x690002
                {
                    status = SPU_PS_Read32(spu_id, 0x04024);

#if 0

                    puts("status = ");
                    print_hex(status);

                    uint32_t npc = SPU_PS_Read32(spu_id, 0x04034);
                    puts(", npc = ");
                    print_hex(npc);

                    uint64_t lslr = SPU_P2_Read64(spu_id, 0x04058);
                    puts(", lslr = ");
                    print_hex(lslr);

                    uint32_t mbox = SPU_PS_Read32(spu_id, 0x04004);
                    puts(", mbox = ");
                    print_hex(mbox);

                    puts("\n");

#if 0

                    for (uint64_t off = 0x39000; off < 0x39200; off += 8)
                    {
                        uint64_t x = SPU_LS_Read64(spu_id, off);

                        print_hex(off);
                        puts(" = ");
                        print_hex(x);
                        puts("\n");
                    }

                    for (uint64_t off = 0x3A000; off < 0x3A200; off += 8)
                    {
                        uint64_t x = SPU_LS_Read64(spu_id, off);

                        print_hex(off);
                        puts(" = ");
                        print_hex(x);
                        puts("\n");
                    }

#endif

                    WaitInMs(1000);

#endif
                }

                // restore MFC_SR1
                SPU_P1_Write64(spu_id, 0x0, old_mfc_sr1);
                eieio();

                //

                *lv1_lv2AreaAddrPtr = 0x8000000000000000;
                *lv1_lv2AreaSizePtr = stage5_lv2AreaSize;

                lv1_lv2AreaHashPtr[0] = SPU_LS_Read64(spu_id, 0x39010);
                lv1_lv2AreaHashPtr[1] = SPU_LS_Read64(spu_id, 0x39018);
                lv1_lv2AreaHashPtr[2] = SPU_LS_Read64(spu_id, 0x39020);

                //

                puts("spu done.\n");
                break;
            }

            if (found == 0)
                puts("free spu not found!!\n");
        }
        else
            puts("file not found!\n");
    }

    puts("after_lv1_lv2AreaAddr = ");
    print_hex(*lv1_lv2AreaAddrPtr);

    puts(", after_lv1_lv2AreaSize = ");
    print_hex(*lv1_lv2AreaSizePtr);

    puts("\n");

    puts("after_lv1_lv2AreaHash[0] = ");
    print_hex(lv1_lv2AreaHashPtr[0]);
    puts("\n");

    puts("after_lv1_lv2AreaHash[1] = ");
    print_hex(lv1_lv2AreaHashPtr[1]);
    puts("\n");

    puts("after_lv1_lv2AreaHash[2] = ");
    print_hex(lv1_lv2AreaHashPtr[2]);
    puts("\n");

    eieio();
}

#pragma GCC pop_options

FUNC_DEF void Stage5()
{
    // puts("BadWDSD Stage5 by Kafuu(aomsin2526)\n");

    // puts("(Build Date: ");
    // puts(__DATE__);
    // puts(" ");
    // puts(__TIME__);
    // puts(")\n");

    Stage5_2();

    // puts("Stage5 done.\n");
}

__attribute__((section("main5"))) void stage5_main()
{
    sc_puts_init();
    Stage5();
}

__attribute__((noreturn, section("entry5"))) void stage5_entry()
{
    register uint64_t r3 asm("r3");

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

    // push stack
    asm volatile("addi 1, 1, -64");

    // store original rtoc to stack
    asm volatile("std 2, 0(1)");

    // store original lr to stack
    asm volatile("mflr %0" : "=r"(r3)::);
    asm volatile("std %0, 8(1)" ::"r"(r3) :);

    // set stage_entry_ra
    asm volatile("bl 4");
    asm volatile("mflr %0" : "=r"(stage_entry_ra)::);
    stage_entry_ra -= (4 * 38);

    // set lv1_rtoc
    asm volatile("mr %0, 2" : "=r"(lv1_rtoc)::);

    // set is_lv1 to 0x9666
    is_lv1 = 0x9666;

    // set stage_zero to 0
    stage_zero = 0;

    // set stage_rtoc
    stage_rtoc = stage_entry_ra;
    stage_rtoc += 0x200; // .toc
    stage_rtoc += 0x8000;

    // set r2 to stage_rtoc
    asm volatile("mr 2, %0" ::"r"(stage_rtoc) :);

    // sync
    asm volatile("sync");

    // jump to stage5_main
    asm volatile("bl stage5_main");

    // restore original lr from stack
    asm volatile("ld %0, 8(1)" : "=r"(r3)::);
    asm volatile("mtlr %0" ::"r"(r3));

    // restore original rtoc from stack
    asm volatile("ld %0, 0(1)" : "=r"(r3)::);
    asm volatile("mr 2, %0" ::"r"(r3));

    // pop stack
    asm volatile("addi 1, 1, 64");

#endif

    // restore all registers from stack
    asm volatile("ld 0, %0(1)" ::"i"(8 * 0) :);
    asm volatile("ld 1, %0(1)" ::"i"(8 * 1) :);
    asm volatile("ld 2, %0(1)" ::"i"(8 * 2) :);
    asm volatile("ld 3, %0(1)" ::"i"(8 * 3) :);
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

    // blr
    asm volatile("blr");

    __builtin_unreachable();
}