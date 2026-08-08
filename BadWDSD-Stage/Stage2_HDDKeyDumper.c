struct HDDKeyDumper_Context_s
{
    uint64_t spu_id;
    uint64_t myspu_id;

    uint8_t sbFix1[24];
    uint8_t sbFix2[24];

    uint8_t iv[16];

    uint8_t hostRndData[32];
    uint64_t hostRndDataSize;

    uint8_t encdecRndData[32];
    uint64_t encdecRndDataSize;

    uint8_t sessionKey[32];

    uint64_t ataDataKeyEa;
    uint8_t ataDataKey[64];

    uint64_t ataTweakKeyEa;
    uint8_t ataTweakKey[64];

    uint64_t encdecDataKeyEa;
    uint8_t encdecDataKey[64];

    uint64_t encdecTweakKeyEa;
    uint8_t encdecTweakKey[64];
};

FUNC_DEF void HDDKeyDumper_Init(struct HDDKeyDumper_Context_s* ctx, uint64_t spu_id)
{
    puts("HDDKeyDumper_Init()\n");

    ctx->spu_id = spu_id;

    puts("spu_id = ");
    print_decimal(ctx->spu_id);
    puts("\n");

    ctx->myspu_id = calc_myspu_id_exclude(ctx->spu_id);

    puts("myspu_id = ");
    print_decimal(ctx->myspu_id);
    puts("\n");

    {
        static const uint64_t stagexSpuElf_MaxSize = (64 * 1024);
        __attribute__((aligned(8))) uint8_t stagexSpuElf[stagexSpuElf_MaxSize];

        SpuAux_CopyElfToMem(stagexSpuElf, stagexSpuElf_MaxSize);

        SpuAux_Init(ctx->myspu_id, stagexSpuElf);
    }

    const uint32_t* sbVersionPtr = (const uint32_t*)0x24000087000;
    uint32_t sbVersion = *sbVersionPtr;

    if ((sbVersion & 0xFF000000) == 0x03000000)
    {
        puts("SB is SX\n");

        uint8_t fix1_sb[] = { 0xC5, 0x12, 0x72, 0xE6, 0x4D, 0x9A, 0x15, 0xCA, 0x44, 0x5C, 0xD2, 0xF3, 0x79, 0x5C, 0x15, 0xB8, 0x1B, 0x19, 0x2C, 0x7A, 0xC1, 0x58, 0x83, 0xF4 };
        uint8_t fix2_sb[] = { 0xE6, 0x18, 0xB3, 0xA4, 0xEC, 0x27, 0x89, 0xDA, 0xFC, 0x2E, 0xD2, 0xDF, 0xED, 0x75, 0xA9, 0x3F, 0xA7, 0xB5, 0x5A, 0xEA, 0x20, 0x12, 0x78, 0xA4 };

        memcpy(ctx->sbFix1, fix1_sb, 24);
        memcpy(ctx->sbFix2, fix2_sb, 24);

        ctx->ataDataKeyEa = 0xc2b0060;
        ctx->ataTweakKeyEa = 0xc2b00c0;

        ctx->encdecDataKeyEa = 0xc2b0120;
        ctx->encdecTweakKeyEa = 0xc2b0180;
    }
    else if ((sbVersion & 0xFF000000) == 0x04000000)
    {
        puts("SB is ZX\n");

        uint8_t fix1_sb[] = { 0xE3, 0xF2, 0x26, 0x65, 0xAF, 0xC4, 0xE1, 0xC0, 0x14, 0xA4, 0x31, 0x24, 0x1D, 0xBC, 0x0B, 0x69, 0xD5, 0xD6, 0x68, 0x57, 0xD9, 0x1E, 0x6B, 0x27 };
        uint8_t fix2_sb[] = { 0x66, 0x86, 0x6A, 0xF7, 0x48, 0x9A, 0xE8, 0x5A, 0xBF, 0x98, 0xA6, 0x70, 0xAA, 0x27, 0x67, 0x2E, 0x06, 0x6E, 0x60, 0xD1, 0x4D, 0x52, 0x41, 0x21 };

        memcpy(ctx->sbFix1, fix1_sb, 24);
        memcpy(ctx->sbFix2, fix2_sb, 24);

        ctx->ataDataKeyEa = 0xc2b00c0;
        ctx->ataTweakKeyEa = 0xc2b0060;

        ctx->encdecDataKeyEa = 0xc2b0180;
        ctx->encdecTweakKeyEa = 0xc2b01e0;
    }
    else
    {
        puts("Unknown SB!!\n");
        dead_beep();
    }

    memset(ctx->iv, 0, 16);

    ctx->hostRndDataSize = 0;
    ctx->encdecRndDataSize = 0;
}

FUNC_DEF void HDDKeyDumper_Finalize(struct HDDKeyDumper_Context_s* ctx)
{
    real_puts("ATA-DATA-KEY:\n");
    real_hexdump(&ctx->ataDataKey[32], 32);
    real_puts("\n");

    real_puts("ATA-TWEAK-KEY:\n");
    real_hexdump(&ctx->ataTweakKey[32], 32);
    real_puts("\n");

    real_puts("ENCDEC-DATA-KEY:\n");
    real_hexdump(&ctx->encdecDataKey[32], 32);
    real_puts("\n");

    real_puts("ENCDEC-TWEAK-KEY:\n");
    real_hexdump(&ctx->encdecTweakKey[32], 32);
    real_puts("\n");

    // for PS3HDH slims
    {
        uint8_t atakey_bin[32];
        memcpy(&atakey_bin[0], &ctx->ataDataKey[32], 16);
        memcpy(&atakey_bin[16], &ctx->ataTweakKey[32], 16);

        real_puts("ata_key.bin (PS3HDH):\n");
        real_hexdump(atakey_bin, 32);
        real_puts("\n");
    }

    {
        uint8_t vflashkey_bin[32];
        memcpy(&vflashkey_bin[0], &ctx->encdecDataKey[32], 16);
        memcpy(&vflashkey_bin[16], &ctx->encdecTweakKey[32], 16);

        real_puts("vflash_key.bin (PS3HDH):\n");
        real_hexdump(vflashkey_bin, 32);
        real_puts("\n");
    }

    sc_write_ata_data_key(&ctx->ataDataKey[32]);
    sc_write_ata_tweak_key(&ctx->ataTweakKey[32]);

    sc_write_encdec_data_key(&ctx->encdecDataKey[32]);
    sc_write_encdec_tweak_key(&ctx->encdecTweakKey[32]);

    sc_write_hdd_key_dumper_flag(0xfe); // dumped

    sc_soft_restart();
}

FUNC_DEF void HDDKeyDumper_ProcessMFCCommand(struct HDDKeyDumper_Context_s* ctx, 
    uint8_t isGet, uint64_t ea, uint64_t ls, const void* dataBuf, uint64_t dataSize)
{
    // host sending rand
    if (ea == 0x24003006000)
    {
        if (isGet || (dataSize != 4))
            dead_beep();

        ctx->hostRndDataSize = 0;
        memcpy(&ctx->hostRndData[ctx->hostRndDataSize], dataBuf, dataSize);
        ctx->hostRndDataSize += dataSize;
    }
    else if ((ctx->hostRndDataSize > 0) && (ctx->hostRndDataSize != 32))
    {
        if (isGet || (dataSize != 4))
            dead_beep();

        memcpy(&ctx->hostRndData[ctx->hostRndDataSize], dataBuf, dataSize);
        ctx->hostRndDataSize += dataSize;

        if (ctx->hostRndDataSize == 32)
        {
            //puts("Computing host rnd...\n");

            spu_aes_decrypt_cbc(ctx->myspu_id, ctx->hostRndData, 32, ctx->hostRndData, ctx->sbFix1, 192, ctx->iv);

            //hexdump(ctx->hostRndData, 32);
            //puts("\n");
        }
    }

    // encdec sending rand
    if (ea == 0x24003006060)
    {
        if (!isGet || (dataSize != 4))
            dead_beep();

        ctx->encdecRndDataSize = 0;
        memcpy(&ctx->encdecRndData[ctx->encdecRndDataSize], dataBuf, dataSize);
        ctx->encdecRndDataSize += dataSize;
    }
    else if ((ctx->encdecRndDataSize > 0) && (ctx->encdecRndDataSize != 32))
    {
        if (!isGet || (dataSize != 4))
            dead_beep();

        memcpy(&ctx->encdecRndData[ctx->encdecRndDataSize], dataBuf, dataSize);
        ctx->encdecRndDataSize += dataSize;

        if (ctx->encdecRndDataSize == 32)
        {
            //puts("Computing encdec rnd...\n");

            spu_aes_decrypt_cbc(ctx->myspu_id, ctx->encdecRndData, 32, ctx->encdecRndData, ctx->sbFix2, 192, ctx->iv);

            //hexdump(ctx->encdecRndData, 32);
            //puts("\n");

            for (uint32_t i = 0; i < 32; ++i)
                ctx->sessionKey[i] = (ctx->hostRndData[i] ^ ctx->encdecRndData[i]);
        }
    }

    // ata data key
    if (ea == ctx->ataDataKeyEa)
    {
        if (isGet || (dataSize != 64))
            dead_beep();

        memcpy(ctx->ataDataKey, dataBuf, 64);
        spu_aes_decrypt_cbc(ctx->myspu_id, ctx->ataDataKey, 64, ctx->ataDataKey, ctx->sessionKey, 192, ctx->iv);
    }

    // ata tweak key
    if (ea == ctx->ataTweakKeyEa)
    {
        if (isGet || (dataSize != 64))
            dead_beep();

        memcpy(ctx->ataTweakKey, dataBuf, 64);
        spu_aes_decrypt_cbc(ctx->myspu_id, ctx->ataTweakKey, 64, ctx->ataTweakKey, ctx->sessionKey, 192, ctx->iv);
    }

    // encdec data key
    if (ea == ctx->encdecDataKeyEa)
    {
        if (isGet || (dataSize != 64))
            dead_beep();

        memcpy(ctx->encdecDataKey, dataBuf, 64);
        spu_aes_decrypt_cbc(ctx->myspu_id, ctx->encdecDataKey, 64, ctx->encdecDataKey, ctx->sessionKey, 192, ctx->iv);
    }

    // encdec tweak key
    if (ea == ctx->encdecTweakKeyEa)
    {
        if (isGet || (dataSize != 64))
            dead_beep();

        memcpy(ctx->encdecTweakKey, dataBuf, 64);
        spu_aes_decrypt_cbc(ctx->myspu_id, ctx->encdecTweakKey, 64, ctx->encdecTweakKey, ctx->sessionKey, 192, ctx->iv);

        // finishing...
        HDDKeyDumper_Finalize(ctx);
    }
}

FUNC_DEF void Stage2_HDDKeyDumper(uint64_t spu_id)
{
    sc_led_short_red_long_yellow();

    puts("Stage2_HDDKeyDumper()\n");

    struct HDDKeyDumper_Context_s context;
    HDDKeyDumper_Init(&context, spu_id);

    SPU_IsoLoadRequest(spu_id);

    // wait for mbox...
    while (1)
    {
        uint32_t mboxStat = SPU_Read_SPU_MBOX_STAT(spu_id);

        // OutMbox
        if ((mboxStat & 0xff) == 1)
        {
            SPU_Read_SPU_OUT_MBOX(spu_id);
            break;
        }
    }

    {
        // 0x80C8080 -> 0x3E800
        memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x3E800), (const void*)0x80C8080, 0x80);
    }

    SPU_EnableRelocate(spu_id);
    SPU_EnableSoftwareTLB(spu_id);
    SPU_EnableLSCmp(spu_id);

    SPU_LS_Write64(spu_id, 0x3E000, 0xFF00000000);

    //uint64_t t1 = GetTimeInMs();

    uint64_t lastTlbIndex = 0;

    while (1)
    {
        //uint64_t t2 = GetTimeInMs();

#if 0

        if ((t2 - t1) > 1000)
        {
            uint32_t status = SPU_Read_SPU_STATUS(spu_id);

            puts("status = ");
            print_hex(status);
            puts("\n");

            uint64_t class0 = SPU_Read_INT_STAT_CLASS0(spu_id);

            puts("class0 = ");
            print_hex(class0);
            puts("\n");

            uint64_t class1 = SPU_Read_INT_STAT_CLASS1(spu_id);

            puts("class1 = ");
            print_hex(class1);
            puts("\n");

            uint64_t class2 = SPU_Read_INT_STAT_CLASS2(spu_id);

            puts("class2 = ");
            print_hex(class2);
            puts("\n");

            t1 = t2;
        }

#endif

        if (SPU_IsSF_INT_STAT_CLASS1(spu_id))
        {
            uint64_t dar = SPU_Read_MFC_DAR(spu_id);

            //puts("DAR = ");
            //print_hex(dar);
            //puts("\n");

            SPU_SetSLB(spu_id, (dar & SPU_SLB_EA_MASK), (dar & SPU_SLB_VA_MASK));

            SPU_ClearSF_INT_STAT_CLASS1(spu_id);
            SPU_Write_MFC_DSISR(spu_id, 0);

            SPU_MFC_ResumeSegmentFault(spu_id);
        }

        if (SPU_IsMF_INT_STAT_CLASS1(spu_id))
        {
            uint64_t dsisr = SPU_Read_MFC_DSISR(spu_id);
            uint64_t dar = SPU_Read_MFC_DAR(spu_id);

            //puts("MFC_DSISR = ");
            //print_hex(SPU_Read_MFC_DSISR(spu_id));
            //puts("\n");

            //puts("DAR = ");
            //print_hex(dar);
            //puts("\n");

            //SPU_PrintAllMFCContextEntry(spu_id);

            uint64_t tmpEa_Base = 0xA000000;
            uint64_t tmpEa = tmpEa_Base;

            if ((dsisr & 0x400000) != 0) // EACmp
            {
                puts("not handled!!!\n");
                dead_beep();
            }
            else if ((dsisr & 0x40000000) != 0)
            {
                //puts("mapping fault!!\n");

                {
                    uint8_t isFoundEntry = 0;
                    //uint8_t foundEntryIdx = 0;

                    struct SPU_MFCContextEntry_s foundEntry;

                    for (uint32_t i = 0; i < 16; ++i)
                    {
                        SPU_ReadMFCContextEntry(spu_id, i, &foundEntry);

                        if ((foundEntry.ea == dar) && (foundEntry.realSize > 0))
                        {
                            isFoundEntry = 1;
                            //foundEntryIdx = i;
                            break;
                        }
                    }

                    if (!isFoundEntry)
                    {
                        puts("entry not found!!!\n");
                        dead_beep();
                    }

                    tmpEa += (foundEntry.ea % 4096);

                    if (foundEntry.command == SPU_MFC_GET_CMD)
                    {
                        //print_hex(foundEntry.ea);
                        //puts(" -> ");
                        //print_hex(foundEntry.ls);

                        //puts(" (");
                        //print_hex(foundEntry.realSize);
                        //puts(") :\n");

                        // tmpEa <- foundEntry.ea
                        memcpy((void*)tmpEa, (const void*)foundEntry.ea, foundEntry.realSize);

                        //hexdump((const void*)tmpEa, foundEntry.realSize);
                        //puts("\n");

                        HDDKeyDumper_ProcessMFCCommand(&context, 1, foundEntry.ea, foundEntry.ls, (const void*)tmpEa, foundEntry.realSize);
                    }
                    else if (foundEntry.command == SPU_MFC_PUT_CMD)
                    {
                        //print_hex(foundEntry.ea);
                        //puts(" <- ");
                        //print_hex(foundEntry.ls);

                        //puts(" (");
                        //print_hex(foundEntry.realSize);
                        //puts(") :\n");

                        // tmpEa <- foundEntry.ls
                        memcpy((void*)tmpEa, (const void*)SPU_CalcMMIOAddress_LS(spu_id, foundEntry.ls), foundEntry.realSize);

                        //hexdump((const void*)tmpEa, foundEntry.realSize);
                        //puts("\n");

                        HDDKeyDumper_ProcessMFCCommand(&context, 0, foundEntry.ea, foundEntry.ls, (const void*)tmpEa, foundEntry.realSize);

                        // foundEntry.ea <- tmpEa
                        memcpy((void*)foundEntry.ea, (const void*)tmpEa, foundEntry.realSize);
                    }
                    else
                    {
                        puts("unknown mfc command!!!\n");
                        dead_beep();
                    }
                }

                lastTlbIndex = SPU_SetTLB(spu_id, (dar & SPU_TLB_VA_MASK), tmpEa_Base);
                //lastTlbIndex = SPU_SetTLB(spu_id, (dar & SPU_TLB_VA_MASK), (dar & SPU_TLB_RA_MASK));
            }
            else
            {
                puts("unknown dsisr!!!\n");
                dead_beep();
            }

            SPU_ClearMF_INT_STAT_CLASS1(spu_id);
            SPU_Write_MFC_DSISR(spu_id, 0);

            SPU_MFC_ResumeMappingFault(spu_id);
        }

        if (SPU_IsLSCmp_INT_STAT_CLASS1(spu_id))
        {
            SPU_InvalidateTLB(spu_id, lastTlbIndex);

            SPU_ClearLSCmp_INT_STAT_CLASS1(spu_id);
            SPU_MFC_ResumeLSCmp(spu_id);
        }
    }

    dead_beep();
}