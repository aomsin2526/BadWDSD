#pragma GCC push_options
#pragma GCC optimize("O0")

FUNC_DEF void SpuAux_Uninit(uint64_t spu_id, uint64_t spu_old_mfc_sr1)
{
    //puts("SpuAux_Uninit()\n");

    //puts("spu_id = ");
    //print_decimal(spu_id);
    //puts("\n");

    uint32_t status = SPU_PS_Read32(spu_id, 0x04024);

    if ((status & SPU_STATUS_RUN_MASK) != 0)
    {
        // stop request
        SPU_PS_Write32(spu_id, 0x0401C, 0x0);
        eieio();

        while ((status & SPU_STATUS_RUN_MASK) != 0)
        {
            status = SPU_PS_Read32(spu_id, 0x04024);
        }
    }

    SPU_P1_Write64(spu_id, 0x0, spu_old_mfc_sr1);
    eieio();

    //puts("SpuAux_Uninit() done.\n");
}

FUNC_DEF uint64_t SpuAux_Init(uint64_t spu_id)
{
    //puts("SpuAux_Init()\n");

    //puts("spu_id = ");
    //print_decimal(spu_id);
    //puts("\n");

    uint64_t spu_old_mfc_sr1 = SPU_P1_Read64(spu_id, 0x0);
    SpuAux_Uninit(spu_id, spu_old_mfc_sr1);

    //

    SPU_P1_Write64(spu_id, 0x0, 0x21);
    eieio();

    //

    uint64_t elfFileAddress = 0;
    uint64_t elfFileSize = 0;

    if (CoreOS_FindFileEntry_Aux("Stagex_spu.elf", &elfFileAddress, &elfFileSize))
    {
        //puts("elfFileAddress = ");
        //print_hex(elfFileAddress);

        //puts(", elfFileSize = ");
        //print_decimal(elfFileSize);

        //puts("\n");

        LoadElfSpu(elfFileAddress, spu_id, 1);
    }
    else
    {
        puts("Stagex_spu.elf not found!\n");
        dead_beep();
    }

    // clear spuReady
    SPU_LS_Write64(spu_id, 0xf10, 0);
    eieio();

    // SPU_RUNCNTL = 0x1
    //puts("Starting spu...\n");
    SPU_PS_Write32(spu_id, 0x0401C, 0x1);
    eieio();

    // wait for spuReady to be 1
    while (SPU_LS_Read64(spu_id, 0xf10) != 1)
    {
        //WaitInMs(1000);

        //status = SPU_PS_Read32(spu_id, 0x04024);

        //puts("status = ");
        //print_hex(status);
        //puts("\n");
    }

    //puts("spuReady ok!\n");

    //puts("SpuAux_Init() done.\n");

    return spu_old_mfc_sr1;
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

    //SpuAux_Init(spu_id);

    // clear jobDone
    SPU_LS_Write64(spu_id, 0xf08, 0);
    eieio();

    {
        {
            struct Stagex_spu_context_s context;
            context.jobType = 1;

            memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &context, sizeof(context));
        }

        {
            struct Stagex_spu_job_aes128_decrypt_ctr_context_s job_context;
            
            memcpy(job_context.key, keys, 16);
            memcpy(job_context.iv, iv, 16);

            job_context.in_ea = (uint64_t)in;
            job_context.out_ea = (uint64_t)out;

            job_context.size = size;

            memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), &job_context, sizeof(job_context));
        }

        eieio();
    }

    // set jobStart
    SPU_LS_Write64(spu_id, 0xf00, 1);
    eieio();

    // wait for jobDone to be 1
    while (SPU_LS_Read64(spu_id, 0xf08) != 1)
    {
        //WaitInMs(1000);

        //uint32_t status = SPU_PS_Read32(spu_id, 0x04024);

        //puts("status = ");
        //print_hex(status);
        //puts("\n");
    }

    //SpuAux_Uninit(spu_id);

    puts("spu_aes128_decrypt_ctr() done.\n");
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
    eieio();

    {
        {
            struct Stagex_spu_context_s context;
            context.jobType = 2;

            memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &context, sizeof(context));
        }

        {
            struct Stagex_spu_job_zlib_decompress_context_s job_context;
            
            job_context.in_ea = (uint64_t)inCompressedData;
            job_context.compressed_size = inCompressedDataSize;

            job_context.out_ea = (uint64_t)outCompressedData;

            memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), &job_context, sizeof(job_context));
        }

        eieio();
    }

    // set jobStart
    SPU_LS_Write64(spu_id, 0xf00, 1);
    eieio();

    // wait for jobDone to be 1
    while (SPU_LS_Read64(spu_id, 0xf08) != 1)
    {
#if 0

        WaitInMs(1000);

        uint32_t status = SPU_PS_Read32(spu_id, 0x04024);

        puts("status = ");
        print_hex(status);
        puts("\n");

        struct Stagex_spu_job_zlib_decompress_context_s job_context;
        memcpy(&job_context, (const void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), sizeof(job_context));

        puts("job_context.in_ea = ");
        print_hex(job_context.in_ea);
        puts("\n");

        puts("job_context.compressed_size = ");
        print_decimal(job_context.compressed_size);
        puts("\n");

        puts("job_context.out_ea = ");
        print_hex(job_context.out_ea);
        puts("\n");

        puts("job_context.out_decompressed_size = ");
        print_decimal(job_context.out_decompressed_size);
        puts("\n");

        puts("job_context.uzlib_cur_in_ea = ");
        print_hex(job_context.uzlib_cur_in_ea);
        puts("\n");

        puts("job_context.uzlib_cur_out_ea = ");
        print_hex(job_context.uzlib_cur_out_ea);
        puts("\n");

        puts("job_context.uzlib_in_left = ");
        print_decimal(job_context.uzlib_in_left);
        puts("\n");

        puts("job_context.uzlib_cur_status = ");
        print_decimal((uint32_t)job_context.uzlib_cur_status);
        puts("\n");

        puts("job_context.uzlib_inTmpBuf = ");
        print_hex(job_context.uzlib_inTmpBuf);
        puts("\n");

        puts("job_context.uzlib_inTmpBufSize = ");
        print_decimal(job_context.uzlib_inTmpBufSize);
        puts("\n");

        puts("job_context.uzlib_outTmpBuf = ");
        print_hex(job_context.uzlib_outTmpBuf);
        puts("\n");

        puts("job_context.uzlib_outTmpBufSize = ");
        print_decimal(job_context.uzlib_outTmpBufSize);
        puts("\n");

        puts("job_context.uzlib_dictTmpBuf = ");
        print_hex(job_context.uzlib_dictTmpBuf);
        puts("\n");

        puts("job_context.uzlib_dictTmpBufSize = ");
        print_decimal(job_context.uzlib_dictTmpBufSize);
        puts("\n");

        struct Stagex_spu_DMACmd_s dmaCmd;
        memcpy(&dmaCmd, (const void*)SPU_CalcMMIOAddress_LS(spu_id, 0x10), sizeof(dmaCmd));

        puts("dmaCmd.ls = ");
        print_hex(dmaCmd.ls);
        puts("\n");

        puts("dmaCmd.ea = ");
        print_hex(dmaCmd.ea);
        puts("\n");

        puts("dmaCmd.size = ");
        print_decimal(dmaCmd.size);
        puts("\n");

        puts("dmaCmd.cmd = ");
        print_hex(dmaCmd.cmd);
        puts("\n");

#endif
    }

    if (outDecompressedDataSize != NULL)
    {
        struct Stagex_spu_job_zlib_decompress_context_s job_context;
        memcpy(&job_context, (const void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), sizeof(job_context));

        *outDecompressedDataSize = job_context.out_decompressed_size;
    }

    puts("spu_zlib_decompress() done.\n");
}

FUNC_DEF void spu_stage3(uint64_t spu_id)
{
    puts("spu_stage3()\n");

    // clear jobDone
    SPU_LS_Write64(spu_id, 0xf08, 0);
    eieio();

    {
        {
            struct Stagex_spu_context_s context;
            context.jobType = 3;

            memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &context, sizeof(context));
        }

        eieio();
    }

    // set jobStart
    SPU_LS_Write64(spu_id, 0xf00, 1);
    eieio();

    // wait for jobDone to be 1
    while (SPU_LS_Read64(spu_id, 0xf08) != 1)
    {

    }

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
    eieio();

    {
        {
            struct Stagex_spu_context_s context;
            context.jobType = 4;

            memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &context, sizeof(context));
        }

        {
            struct Stagex_spu_job_DecryptLv0Self_context_s context;

            context.inDestEa = (uint64_t)inDest;
            context.inSrcEa = (uint64_t)inSrc;

            memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), &context, sizeof(context));
        }

        eieio();
    }

    // set jobStart
    SPU_LS_Write64(spu_id, 0xf00, 1);
    eieio();

    // wait for jobDone to be 1
    while (SPU_LS_Read64(spu_id, 0xf08) != 1)
    {

    }

    puts("SPU_DecryptLv0Self() done.\n");
}

FUNC_DEF void spu_stage2(uint64_t spu_id)
{
    puts("spu_stage2()\n");

    // clear jobDone
    SPU_LS_Write64(spu_id, 0xf08, 0);
    eieio();

    {
        {
            struct Stagex_spu_context_s context;
            context.jobType = 5;

            memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &context, sizeof(context));
        }

        eieio();
    }

    // set jobStart
    SPU_LS_Write64(spu_id, 0xf00, 1);
    eieio();

    // wait for jobDone to be 1
    while (SPU_LS_Read64(spu_id, 0xf08) != 1)
    {

    }

    puts("spu_stage2() done.\n");
}

#pragma GCC pop_options