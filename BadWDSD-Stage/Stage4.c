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

    struct Stagex_Context_s* ctx = GetStagexContext();

    uint64_t srcAddr = 0xC000000;

    if (ctx->stage3_ignoreSrc == 0x3333) // src ignored
    {
        if (!CoreOS_FindFileEntry_CurrentBank("lv2_kernel.self", &srcAddr, NULL))
        {
            lv1_puts("lv2_kernel.self not found!\n");
            dead_beep();
        }
    }

    LoadLv2(srcAddr, loadme_addr);

    lv1_puts("Stage4 done.\n");

    eieio();
}