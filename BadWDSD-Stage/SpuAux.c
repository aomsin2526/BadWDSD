FUNC_DEF void SpuAux_CopyElfToMem(void* elfFileData, uint64_t elfFileData_MaxSize)
{
    uint32_t fileFlashOffset = 0;
    uint32_t fileSize = 0;

    if (!CoreOS2_FindFileEntry_Aux("Stagex_spu.elf", &fileFlashOffset, &fileSize))
    {
        puts("Stagex_spu.elf not found!\n");
        dead_beep();
    }

    if (fileSize > elfFileData_MaxSize)
    {
        puts("Stagex_spu.elf too big!\n");
        dead_beep();
    }

    FlashRead(fileFlashOffset, elfFileData, fileSize);
}

FUNC_DEF void SpuAux_Uninit(uint64_t spu_id, uint64_t spu_old_mfc_sr1)
{
    //puts("SpuAux_Uninit()\n");

    //puts("spu_id = ");
    //print_decimal(spu_id);
    //puts("\n");

    SPU_StopRequest(spu_id);
    SPU_IsoExitRequest(spu_id);

    SPU_Write_MFC_SR1(spu_id, spu_old_mfc_sr1);

    //puts("SpuAux_Uninit() done.\n");
}

FUNC_DEF uint64_t SpuAux_Init(uint64_t spu_id, const void* elfFileData)
{
    //puts("SpuAux_Init()\n");

    if (elfFileData == NULL)
    {
        puts("elfFileData is null!\n");
        dead_beep();
    }

    //puts("spu_id = ");
    //print_decimal(spu_id);
    //puts("\n");

    uint64_t spu_old_mfc_sr1 = SPU_Read_MFC_SR1(spu_id);
    SpuAux_Uninit(spu_id, spu_old_mfc_sr1);

    //

    SPU_Write_MFC_SR1(spu_id, 0x21);

    //

    LoadElfSpu((uint64_t)elfFileData, spu_id, 1);

    // clear spuReady
    SPU_LS_Write64(spu_id, 0xf10, 0);

    //puts("Starting spu...\n");
    SPU_StartRequest(spu_id);

    // wait for spuReady to be 1
    while (SPU_LS_Read64(spu_id, 0xf10) != 1) {}

    //puts("spuReady ok!\n");

    //puts("SpuAux_Init() done.\n");
    return spu_old_mfc_sr1;
}

FUNC_DEF uint64_t SpuAux_Init_lv1(uint64_t spu_id)
{
    puts("SpuAux_Init_lv1()\n");

    if (!IsLv1())
    {
        puts("lv1 only!\n");
        dead_beep();
    }

    const struct Stagex_Context_s* ctx = GetStagexContext();

    if (ctx->cached_StagexSpuElf_FileSize > stagex_aux_max_size)
    {
        puts("too big!\n");
        dead_beep();
    }

    uint8_t* elfFileData = (uint8_t*)0xD000000;
    FlashRead(ctx->cached_StagexSpuElf_FileFlashOffset, elfFileData, ctx->cached_StagexSpuElf_FileSize);

    uint64_t v = SpuAux_Init(spu_id, elfFileData);

    puts("SpuAux_Init_lv1() done\n");
    return v;
}

struct __attribute__((aligned(8))) Stagex_spu_DMACmd_s
{
    uint32_t ls;
    uint64_t ea;

    uint16_t size;

    uint16_t cmd;
};

struct __attribute__((aligned(8))) Stagex_spu_context_s
{
    uint32_t jobType;
};

struct __attribute__((aligned(8))) Stagex_spu_job_aes128_decrypt_ctr_context_s
{
    uint8_t key[16];
    uint8_t iv[16];

    uint64_t in_ea;
    uint64_t out_ea;

    uint64_t size; // size to decrypt
};

// keys[16]
// iv[16]
FUNC_DEF void spu_aes128_decrypt_ctr(uint64_t spu_id, const uint8_t* in, uint64_t size, uint8_t* out, const uint8_t* keys, const uint8_t* iv)
{
    puts("spu_aes128_decrypt_ctr()\n");

    if (size == 0)
        return;

    // clear jobDone
    SPU_LS_Write64(spu_id, 0xf08, 0);

    {
        {
            struct Stagex_spu_context_s context;
            context.jobType = 1;

            volatile_memcpy((volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &context, sizeof(context));
        }

        {
            struct Stagex_spu_job_aes128_decrypt_ctr_context_s job_context;
            
            memcpy(job_context.key, keys, 16);
            memcpy(job_context.iv, iv, 16);

            job_context.in_ea = (uint64_t)in;
            job_context.out_ea = (uint64_t)out;

            job_context.size = size;

            volatile_memcpy((volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), &job_context, sizeof(job_context));
        }
    }

    // set jobStart
    SPU_LS_Write64(spu_id, 0xf00, 1);

    // wait for jobDone to be 1
    while (SPU_LS_Read64(spu_id, 0xf08) != 1) {}

    puts("spu_aes128_decrypt_ctr() done.\n");
}

struct __attribute__((aligned(8))) Stagex_spu_job_aes_decrypt_cbc_context_s
{
    uint8_t key[32];
    uint32_t key_size;

    uint8_t iv[16];

    uint64_t in_ea;
    uint64_t out_ea;

    uint64_t size; // size to decrypt
};

// key_size in bits
FUNC_DEF void spu_aes_decrypt_cbc(uint64_t spu_id, const uint8_t* in, uint64_t size, uint8_t* out, const uint8_t* keys, uint32_t key_size, const uint8_t* iv)
{
    puts("spu_aes_decrypt_cbc()\n");

    if (size == 0)
        return;

    // clear jobDone
    SPU_LS_Write64(spu_id, 0xf08, 0);

    {
        {
            struct Stagex_spu_context_s context;
            context.jobType = 6;

            volatile_memcpy((volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &context, sizeof(context));
        }

        {
            struct Stagex_spu_job_aes_decrypt_cbc_context_s job_context;

            memcpy(job_context.key, keys, (key_size / 8));
            job_context.key_size = key_size;

            memcpy(job_context.iv, iv, 16);

            job_context.in_ea = (uint64_t)in;
            job_context.out_ea = (uint64_t)out;

            job_context.size = size;

            volatile_memcpy((volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), &job_context, sizeof(job_context));
        }
    }

    // set jobStart
    SPU_LS_Write64(spu_id, 0xf00, 1);

    // wait for jobDone to be 1
    while (SPU_LS_Read64(spu_id, 0xf08) != 1) {}

    puts("spu_aes_decrypt_cbc() done.\n");
}

struct __attribute__((aligned(8))) Stagex_spu_job_zlib_decompress_context_s
{
    uint64_t in_ea;
    uint64_t compressed_size;

    uint64_t out_ea;
    uint64_t out_decompressed_size; // output written by spu

    // internal context, exposed to ppu for debugging

    uint64_t uzlib_cur_in_ea;
    uint64_t uzlib_cur_out_ea;

    uint64_t uzlib_in_left;

    int32_t uzlib_cur_status;

    uint32_t uzlib_inTmpBuf; // uint8_t*
    uint32_t uzlib_inTmpBufSize;

    uint32_t uzlib_outTmpBuf; // uint8_t*
    uint32_t uzlib_outTmpBufSize;

    uint32_t uzlib_dictTmpBuf; // uint8_t*
    uint32_t uzlib_dictTmpBufSize;
};

FUNC_DEF void spu_zlib_decompress(uint64_t spu_id, const void* inCompressedData, uint64_t inCompressedDataSize, void* outCompressedData, uint64_t* outDecompressedDataSize)
{
    puts("spu_zlib_decompress()\n");

    // clear jobDone
    SPU_LS_Write64(spu_id, 0xf08, 0);

    {
        {
            struct Stagex_spu_context_s context;
            context.jobType = 2;

            volatile_memcpy((volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &context, sizeof(context));
        }

        {
            struct Stagex_spu_job_zlib_decompress_context_s job_context;
            
            job_context.in_ea = (uint64_t)inCompressedData;
            job_context.compressed_size = inCompressedDataSize;

            job_context.out_ea = (uint64_t)outCompressedData;

            volatile_memcpy((volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), &job_context, sizeof(job_context));
        }
    }

    // set jobStart
    SPU_LS_Write64(spu_id, 0xf00, 1);

    // wait for jobDone to be 1
    while (SPU_LS_Read64(spu_id, 0xf08) != 1) {}

    if (outDecompressedDataSize != NULL)
    {
        struct Stagex_spu_job_zlib_decompress_context_s job_context;
        volatile_memcpy(&job_context, (const volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), sizeof(job_context));

        *outDecompressedDataSize = job_context.out_decompressed_size;
    }

    puts("spu_zlib_decompress() done.\n");
}

FUNC_DEF void spu_stage3(uint64_t spu_id)
{
    puts("spu_stage3()\n");

    // clear jobDone
    SPU_LS_Write64(spu_id, 0xf08, 0);

    {
        {
            struct Stagex_spu_context_s context;
            context.jobType = 3;

            volatile_memcpy((volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &context, sizeof(context));
        }
    }

    // set jobStart
    SPU_LS_Write64(spu_id, 0xf00, 1);

    // wait for jobDone to be 1
    while (SPU_LS_Read64(spu_id, 0xf08) != 1) {}

    puts("spu_stage3() done.\n");
}

struct __attribute__((aligned(8))) Stagex_spu_job_DecryptLv0Self_context_s
{
    uint64_t inDestEa;
    uint64_t inSrcEa;
};

FUNC_DEF void SPU_DecryptLv0Self(uint64_t spu_id, void* inDest, const void* inSrc)
{
    puts("SPU_DecryptLv0Self()\n");

    // clear jobDone
    SPU_LS_Write64(spu_id, 0xf08, 0);

    {
        {
            struct Stagex_spu_context_s context;
            context.jobType = 4;

            volatile_memcpy((volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &context, sizeof(context));
        }

        {
            struct Stagex_spu_job_DecryptLv0Self_context_s context;

            context.inDestEa = (uint64_t)inDest;
            context.inSrcEa = (uint64_t)inSrc;

            volatile_memcpy((volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), &context, sizeof(context));
        }
    }

    // set jobStart
    SPU_LS_Write64(spu_id, 0xf00, 1);

    // wait for jobDone to be 1
    while (SPU_LS_Read64(spu_id, 0xf08) != 1) {}

    puts("SPU_DecryptLv0Self() done.\n");
}

struct __attribute__((aligned(8))) Stagex_spu_job_stage2_context_s
{
    uint8_t is_qcfw_jig;

    uint8_t patch_aim;
    uint8_t patch_inspect_package_tophalf;

    uint8_t fsm_toggle;
};

FUNC_DEF void spu_stage2(uint64_t spu_id, const struct Stagex_spu_job_stage2_context_s* job_context)
{
    puts("spu_stage2()\n");

    // clear jobDone
    SPU_LS_Write64(spu_id, 0xf08, 0);

    {
        {
            struct Stagex_spu_context_s context;
            context.jobType = 5;

            volatile_memcpy((volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &context, sizeof(context));
        }

        {
            volatile_memcpy((volatile void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), job_context, sizeof(struct Stagex_spu_job_stage2_context_s));
        }
    }

    // set jobStart
    SPU_LS_Write64(spu_id, 0xf00, 1);

    // wait for jobDone to be 1
    while (SPU_LS_Read64(spu_id, 0xf08) != 1) {}

    puts("spu_stage2() done.\n");
}