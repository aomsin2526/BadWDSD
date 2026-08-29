FUNC_DEF uint64_t FindLoadMe2(uint64_t addr)
{
    const uint32_t *v = (const uint32_t *)addr;

    if (v[0] == 0x4C4F4144 && v[1] == 0x4D455858 && v[2] == 0x58584C4F && v[3] == 0x41444D45)
        return addr;

    return 0;
}

FUNC_DEF uint64_t FindLoadMe()
{
    uint64_t loadme_addr = 0;

    if (loadme_addr == 0)
        loadme_addr = FindLoadMe2(0x8000000);
    if (loadme_addr == 0)
        loadme_addr = FindLoadMe2(0x1000000);
    
    if (loadme_addr == 0)
    {
        for (uint64_t addr = 0x2000000; addr < (256 * 1024 * 1024); addr += 0x1000000)
        {
            loadme_addr = FindLoadMe2(addr);

            if (loadme_addr != 0)
                break;
        }
    }

    return loadme_addr;
}

FUNC_DEF void LoadLv2(uint64_t srcAddr, uint64_t loadme_addr)
{
    lv1_puts("srcAddr = ");
    lv1_print_hex(srcAddr);
    lv1_puts("\n");

    lv1_puts("loadme_addr = ");
    lv1_print_hex(loadme_addr);
    lv1_puts("\n");

    //hexdump((const void*)srcAddr, 0x1000);

    {
        const struct SceHeader_s *sceHeader = (const struct SceHeader_s *)srcAddr;

        if (sceHeader->magic != 0x53434500)
        {
            lv1_puts("sce_magic check failed!\n");
            dead_beep();
        }

        if (sceHeader->attribute != 0x8000)
        {
            lv1_puts("SELF detected\n");

#if DECRYPTLV2SELF_ENABLED

            //FUNC_DEF void DecryptLv2Self(void *inDest, const void *inSrc, void* decryptBuf)
            DecryptLv2Self((void*)0xB000000, (const void*)srcAddr, (void*)0xA000000, 1);

            lv1_puts("Loading elf...\n");

            memset((void *)loadme_addr, 0, (16 * 1024 * 1024));
            LoadElf(0xB000000, loadme_addr, 1);

#else
            lv1_puts("not supported!\n");
            dead_beep();
#endif

        }
        else
        {
            uint64_t file_offset = *((const uint64_t *)(srcAddr + 0x10));

            lv1_puts("file_offset = ");
            lv1_print_hex(file_offset);

            uint64_t real_file_size = *((const uint64_t *)(srcAddr + 0x18));

            lv1_puts(", real_file_size = ");
            lv1_print_hex(real_file_size);

            uint64_t real_file_data = (srcAddr + file_offset);

            lv1_puts(", real_file_data = ");
            lv1_print_hex(real_file_data);

            uint64_t file_size = real_file_size;
            file_size -= 0x10000;

            lv1_puts(", file_size = ");
            lv1_print_hex(file_size);

            uint64_t file_data = real_file_data;
            file_data += 0x10000;

            lv1_puts(", file_data = ");
            lv1_print_hex(file_data);

            lv1_puts("\n");

            uint64_t zelf_magic = *((const uint64_t *)file_data);

            if ((zelf_magic == 0x5A454C465A454C46) || (zelf_magic == 0x5A454C465A454C32))
            {
                lv1_puts("ZELF/ZELF2 detected\n");

                uint64_t sz = (16 * 1024 * 1024);
                ZelfDecompress(file_data, (void *)0xB000000, &sz, 1, NULL);

                lv1_puts("Loading elf...\n");

                memset((void *)loadme_addr, 0, (16 * 1024 * 1024));
                LoadElf(0xB000000, loadme_addr, 1);
            }
            else
            {
                lv1_puts("ZELF not detected, assume RAW\n");

                memset((void *)loadme_addr, 0, (16 * 1024 * 1024));
                memcpy((void *)loadme_addr, (const void *)file_data, file_size);
            }
        }
    }

    {
        uint8_t qcfw_lite_flag = get_qcfw_lite_flag();

        if (qcfw_lite_flag == 0x1)
            EnableLoadCobraFromUSB(loadme_addr);
    }
}

FUNC_DEF void Stage4()
{
    sc_puts_init();

    lv1_puts("BadWDSD Stage4 by Kafuu(aomsin2526)\n");

    lv1_puts("(Build Date: ");
    lv1_puts(__DATE__);
    lv1_puts(" ");
    lv1_puts(__TIME__);
    lv1_puts(")\n");

    uint64_t loadme_addr = FindLoadMe();

    lv1_puts("loadme_addr = ");
    lv1_print_hex(loadme_addr);
    lv1_puts("\n");

    if (loadme_addr == 0)
    {
        lv1_puts("can't find loadme!!\n");
        dead_beep();
    }

    *((uint64_t *)loadme_addr) = 0;

    uint64_t srcAddr = 0xC000000;
    LoadLv2(srcAddr, loadme_addr);

    if (IsLv2(loadme_addr))
        ApplyLv2Diff(loadme_addr, 1, 0);

    lv1_puts("Stage4 done.\n");
}