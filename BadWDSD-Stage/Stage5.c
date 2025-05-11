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

FUNC_DEF uint64_t SPU_LS_Read64(uint64_t spu_id, uint64_t offset)
{
    return *((uint64_t *)SPU_CalcMMIOAddress_LS(spu_id, offset));
}

FUNC_DEF void SPU_LS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((uint64_t *)SPU_CalcMMIOAddress_LS(spu_id, offset)) = value;
}

FUNC_DEF uint32_t SPU_LS_Read32(uint64_t spu_id, uint64_t offset)
{
    return *((uint32_t *)SPU_CalcMMIOAddress_LS(spu_id, offset));
}

FUNC_DEF void SPU_LS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((uint32_t *)SPU_CalcMMIOAddress_LS(spu_id, offset)) = value;
}

FUNC_DEF uint64_t SPU_PS_Read64(uint64_t spu_id, uint64_t offset)
{
    return *((uint64_t *)SPU_CalcMMIOAddress_PS(spu_id, offset));
}

FUNC_DEF void SPU_PS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((uint64_t *)SPU_CalcMMIOAddress_PS(spu_id, offset)) = value;
}

FUNC_DEF uint32_t SPU_PS_Read32(uint64_t spu_id, uint64_t offset)
{
    return *((uint32_t *)SPU_CalcMMIOAddress_PS(spu_id, offset));
}

FUNC_DEF void SPU_PS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((uint32_t *)SPU_CalcMMIOAddress_PS(spu_id, offset)) = value;
}

FUNC_DEF uint64_t SPU_P2_Read64(uint64_t spu_id, uint64_t offset)
{
    return *((uint64_t *)SPU_CalcMMIOAddress_P2(spu_id, offset));
}

FUNC_DEF void SPU_P2_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((uint64_t *)SPU_CalcMMIOAddress_P2(spu_id, offset)) = value;
}

FUNC_DEF uint32_t SPU_P2_Read32(uint64_t spu_id, uint64_t offset)
{
    return *((uint32_t *)SPU_CalcMMIOAddress_P2(spu_id, offset));
}

FUNC_DEF void SPU_P2_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((uint32_t *)SPU_CalcMMIOAddress_P2(spu_id, offset)) = value;
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
    SPU_PS_Write32(spu_id, 0x04034, (elfHdr->e_entry << 0));

    eieio();

    puts("LoadElfSpu() done.\n");
}

FUNC_DEF void Stage5()
{
    puts("BadWDSD Stage5 by Kafuu(aomsin2526)\n");

    puts("(Build Date: ");
    puts(__DATE__);
    puts(" ");
    puts(__TIME__);
    puts(")\n");

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

        puts("Searching for appldr.elf...\n");

        uint64_t appldrFileAddress;
        uint64_t appldrFileSize;

        if (CoreOS_FindFileEntry(coreOSStartAddress, "appldr.elf", &appldrFileAddress, &appldrFileSize))
        {
            puts("appldrFileAddress = ");
            print_hex(appldrFileAddress);

            puts(", appldrFileSize = ");
            print_decimal(appldrFileSize);

            puts("\n");

            // Sc_Rx: Scanning for spu id 0...
            // Sc_Rx: status = 0x2, runcntl = 0x2, privcntl = 0x0...
            // Sc_Rx: Scanning for spu id 1...
            // Sc_Rx: status = 0x2, runcntl = 0x2, privcntl = 0x0...
            // Sc_Rx: Scanning for spu id 2...
            // Sc_Rx: status = 0x2, runcntl = 0x2, privcntl = 0x0...
            // Sc_Rx: Scanning for spu id 4...
            // Sc_Rx: status = 0x81, runcntl = 0x1, privcntl = 0x4...
            // Sc_Rx: Scanning for spu id 5...
            // Sc_Rx: status = 0x2, runcntl = 0x2, privcntl = 0x0...
            // Sc_Rx: Scanning for spu id 6...
            // Sc_Rx: status = 0x1100002, runcntl = 0x1, privcntl = 0x0...
            // Sc_Rx: Scanning for spu id 7...
            // Sc_Rx: status = 0x2, runcntl = 0x2, privcntl = 0x0...

            intr_disable();

            for (uint64_t spu_id = 0; spu_id < 8; ++spu_id)
            {
                if (spu_id == 3)
                    continue;

                puts("Scanning for isolated spu id ");
                print_decimal(spu_id);
                puts("...\n");

                uint32_t status = SPU_PS_Read32(spu_id, 0x04024);
                puts("status = ");
                print_hex(status);

                uint32_t runcntl = SPU_PS_Read32(spu_id, 0x0401C);
                puts(", runcntl = ");
                print_hex(runcntl);

                uint64_t privcntl = SPU_P2_Read64(spu_id, 0x04040);
                puts(", privcntl = ");
                print_hex(privcntl);

                puts("...\n");

                static const uint32_t SPU_STATUS_RUN_MASK = (1 << 0);
                static const uint32_t SPU_STATUS_ISOLATED_MASK = (1 << 7);
                static const uint32_t SPU_STATUS_ISOLATED_LOAD_MASK = (1 << 9);

                while ((status & SPU_STATUS_ISOLATED_LOAD_MASK) != 0)
                {
                    status = SPU_PS_Read32(spu_id, 0x04024);
                }

                if (((status & SPU_STATUS_ISOLATED_MASK) != 0) && ((status & SPU_STATUS_RUN_MASK) != 0))
                {
                    puts("Found!\n");

                    // stop request
                    SPU_PS_Write32(spu_id, 0x0401C, 0x0);
                    eieio();

                    // wait until spu stop
                    puts("Waiting for spu stop...\n");
                    while ((status & SPU_STATUS_RUN_MASK) != 0)
                    {
                        status = SPU_PS_Read32(spu_id, 0x04024);
                    }

                    puts("spu stopped.\n");

                    // isolation exit
                    SPU_PS_Write32(spu_id, 0x0401C, 0x2);
                    eieio();

                    // wait until spu exit isolation
                    puts("Waiting for spu isolation exit...\n");
                    while ((status & SPU_STATUS_ISOLATED_MASK) != 0)
                    {
                        status = SPU_PS_Read32(spu_id, 0x04024);
                    }

                    puts("spu isolation exited.\n");

                    //

                    LoadElfSpu(appldrFileAddress, spu_id);
                    eieio();

                    // SPU_RUNCNTL = 0x1
                    SPU_PS_Write32(spu_id, 0x0401C, 0x1);
                    eieio();

                    // wait until spu start
                    puts("Waiting for spu start...\n");
                    //while ((status & SPU_STATUS_RUN_MASK) == 0)
                    while (1)
                    {
                        status = SPU_PS_Read32(spu_id, 0x04024);

                        uint32_t status = SPU_PS_Read32(spu_id, 0x04024);
                        puts("status = ");
                        print_hex(status);

                        WaitInMs(1000);

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
                    }

                    break;
                }
            }

            intr_enable();
        }
        else
            puts("file not found!\n");
    }

    puts("Stage5 done.\n");
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
    stage_rtoc += 0x500; // .toc
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