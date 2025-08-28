FUNC_DEF uint64_t FindLoadMe()
{
    uint64_t loadme_addr = 0;

    // 4C4F4144 4D455858 58584C4F 41444D45

    for (uint64_t addr = 0; addr < (240 * 1024 * 1024); addr += 4)
    {
        const uint32_t *v = (const uint32_t *)addr;

        if (v[0] == 0x4C4F4144 && v[1] == 0x4D455858 && v[2] == 0x58584C4F && v[3] == 0x41444D45)
        {
            loadme_addr = addr;
            break;
        }
    }

    return loadme_addr;
}

FUNC_DEF void Stage4()
{
    sc_puts_init();

    puts("BadWDSD Stage4 by Kafuu(aomsin2526)\n");

    puts("(Build Date: ");
    puts(__DATE__);
    puts(" ");
    puts(__TIME__);
    puts(")\n");

    uint64_t loadme_addr = FindLoadMe();

    puts("loadme_addr = ");
    print_hex(loadme_addr);
    puts("\n");

    if (loadme_addr == 0)
    {
        puts("can't find loadme!!\n");
        dead_beep();
    }

    *((uint64_t *)loadme_addr) = 0;

    {
        struct SceHeader_s *sceHeader = (struct SceHeader_s *)0xC000000;

        if (sceHeader->magic != 0x53434500)
        {
            puts("sce_magic check failed!\n");
            dead_beep();
        }

        if (sceHeader->attribute != 0x8000)
        {
            puts("SELF detected\n");

            //FUNC_DEF void DecryptLv2Self(void *inDest, const void *inSrc, void* decryptBuf)
            DecryptLv2Self((void*)0xB000000, (const void*)0xC000000, (void*)0xA000000);

            puts("Loading elf...\n");

            memset((void *)loadme_addr, 0, (16 * 1024 * 1024));
            LoadElf(0xB000000, loadme_addr, 1);
        }
        else
        {
            uint64_t file_offset = *((uint64_t *)0xC000010);

            puts("file_offset = ");
            print_hex(file_offset);

            uint64_t real_file_size = *((uint64_t *)0xC000018);

            puts(", real_file_size = ");
            print_hex(real_file_size);

            uint64_t real_file_data = (0xC000000 + file_offset);

            puts(", real_file_data = ");
            print_hex(real_file_data);

            uint64_t file_size = real_file_size;
            file_size -= 0x10000;

            puts(", file_size = ");
            print_hex(file_size);

            uint64_t file_data = real_file_data;
            file_data += 0x10000;

            puts(", file_data = ");
            print_hex(file_data);

            puts("\n");

            uint64_t zelf_magic = *((uint64_t *)file_data);

            if (zelf_magic == 0x5A454C465A454C46)
            {
                puts("ZELF detected\n");

                uint64_t sz = (16 * 1024 * 1024);
                ZelfDecompress(file_data, (void *)0xB000000, &sz);

                puts("Loading elf...\n");

                memset((void *)loadme_addr, 0, (16 * 1024 * 1024));
                LoadElf(0xB000000, loadme_addr, 1);
            }
            else
            {
                puts("ZELF not detected, assume RAW\n");

                memset((void *)loadme_addr, 0, (16 * 1024 * 1024));
                memcpy((void *)loadme_addr, (void *)file_data, file_size);
            }
        }
    }

    if (0)
    {
        puts("Enabling Load Cobra from USB...\n");

        volatile uint8_t* enableLoadCobraFromUSB = (volatile uint8_t*)(loadme_addr + 0x30);
        *enableLoadCobraFromUSB = 1;
        eieio();

        sc_triple_beep();
        WaitInMs(1200);
    }

    puts("Stage4 done.\n");

    eieio();
}