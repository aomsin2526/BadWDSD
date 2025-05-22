#pragma GCC push_options
//#pragma GCC optimize("O0")

FUNC_DEF void Stage6()
{
    puts("Stage6!!!\n");

    //return;

    {
        puts("Searching for myappldr.elf...\n");

        uint64_t myappldrFileAddress;
        uint64_t myappldrFileSize;

        if (CoreOS_FindFileEntry_CurrentBank("myappldr.elf", &myappldrFileAddress, &myappldrFileSize))
        {
            puts("myappldrFileAddress = ");
            print_hex(myappldrFileAddress);

            puts(", myappldrFileSize = ");
            print_decimal(myappldrFileSize);

            puts("\n");

            //for (uint64_t spu_id = 0; spu_id < 8; ++spu_id)
            uint64_t spu_id = 4;
            {
                //if (spu_id == 3)
                    //continue;

                puts("Scanning for isolated spu id ");
                print_decimal(spu_id); // likely to be spu4
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

                //if (((status & SPU_STATUS_ISOLATED_MASK) != 0) && ((status & SPU_STATUS_RUN_MASK) != 0))
                {
                    puts("Found!\n");

                    uint64_t* backup_ls = (uint64_t*)0xFFF0000;

                    // back up ls

                    puts("Backing up ls...\n");

                    for (uint64_t i = 0; i < (8192 / 8); ++i)
                        backup_ls[i] = SPU_LS_Read64(spu_id, (0x3E000 + (i * 8)));

                    eieio();

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

                    puts("Loading programs...\n");

                    LoadElfSpu(myappldrFileAddress, spu_id);
                    eieio();

                    // filling parameters...

                    puts("Filling parameters...\n");

                    {
                        for (uint64_t i = 0; i < (8192 / 8); ++i)
                            SPU_LS_Write64(spu_id, (0x3E000 + (i * 8)), backup_ls[i]);

                        eieio();
                    }

                    {
                        // keyring

                        SPU_LS_Write64(spu_id, 0x39000, 0xF504FD920DFD54B7);
                        SPU_LS_Write64(spu_id, 0x39000 + 8, 0x38FC889500000000);
                    }

                    {
                        // idps

                        SPU_LS_Write64(spu_id, 0x39010, 0x0000000100830006);
                        SPU_LS_Write64(spu_id, 0x39010 + 8, 0x1004C09929B6048F);
                    }

                    {
                        // tid

                        SPU_LS_Write64(spu_id, 0x39020, 0x83);
                    }

#if 0

                    {
                        // debug stop 0x69
                        SPU_LS_Write64(spu_id, 0x12C00, 0x0000006900000000);
                    }

#endif

                    eieio();

                    // SPU_RUNCNTL = 0x1
                    puts("Starting spu...\n");
                    SPU_PS_Write32(spu_id, 0x0401C, 0x1);
                    eieio();

                    // wait until spu start
                    puts("Waiting for spu start...\n");
                    //while ((status & SPU_STATUS_RUN_MASK) == 0)
                    while (1)
                    {
                        // debug code...

                        WaitInMs(1000);

                        status = SPU_PS_Read32(spu_id, 0x04024);
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

                        {
                            uint64_t x = SPU_LS_Read64(spu_id, 0x37300);
                            puts(", 0x37300 = ");
                            print_hex(x);
                        }

                        {
                            uint64_t x = SPU_LS_Read64(spu_id, 0x39000);
                            puts(", 0x39000 = ");
                            print_hex(x);
                        }

                        puts("\n");

                        break;
                    }

                    //break;
                }
            }
        }
        else
            puts("File not found!\n");
    }

    puts("Stage6 done.\n");
}

#pragma GCC pop_options

__attribute__((section("main6"))) void stage6_main()
{
    uint64_t* isActive = (uint64_t*)0x228;
    
    if (*isActive == 0)
        return;

    sc_puts_init();
    Stage6();
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

    register uint64_t r3 asm("r3");

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

    // set interrupt_depth to 0
    interrupt_depth = 0;

    // set is_lv1 to 0x9666 (stage5)
    is_lv1 = 0x9666;

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