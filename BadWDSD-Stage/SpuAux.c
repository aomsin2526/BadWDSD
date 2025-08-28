#pragma GCC push_options
#pragma GCC optimize("O0")

FUNC_DEF void SpuAux_Uninit(uint64_t spu_id)
{
    //puts("SpuAux_Uninit()\n");

    //puts("spu_id = ");
    //print_decimal(spu_id);
    //puts("\n");

    static const uint32_t SPU_STATUS_RUN_MASK = (1 << 0);

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

    //puts("SpuAux_Uninit() done.\n");
}

FUNC_DEF void SpuAux_Init(uint64_t spu_id)
{
    //puts("SpuAux_Init()\n");

    //puts("spu_id = ");
    //print_decimal(spu_id);
    //puts("\n");

    SpuAux_Uninit(spu_id);

    //static const uint32_t SPU_STATUS_RUN_MASK = (1 << 0);
    static const uint32_t SPU_STATUS_ISOLATED_MASK = (1 << 7);

    uint32_t status = SPU_PS_Read32(spu_id, 0x04024);
 
    if ((status & SPU_STATUS_ISOLATED_MASK) != 0)
    {
        //puts("iso exit\n");

        // isolation exit
        SPU_PS_Write32(spu_id, 0x0401C, 0x2);
        eieio();

        while ((status & SPU_STATUS_ISOLATED_MASK) != 0)
        {
            status = SPU_PS_Read32(spu_id, 0x04024);
        }

        //puts("iso exit done.\n");
    }

    uint64_t elfFileAddress = 0;
    uint64_t elfFileSize = 0;

    if (CoreOS_FindFileEntry(0x2401FF21000, "Stagex_spu.elf", &elfFileAddress, &elfFileSize))
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
}

__attribute__((aligned(8))) struct Stagex_spu_params_s
{
    uint32_t jobType; // 1 = aes128_decrypt_ctr
};

__attribute__((aligned(8))) struct Stagex_spu_job_aes128_decrypt_ctr_params_s
{
    uint8_t key[16];
    uint8_t iv[16];

    uint64_t in_ea;
    uint64_t out_ea;

    uint64_t size; // size to decrypt
};

// keys[16]
// iv[16]
FUNC_DEF void spu_aes128_decrypt_ctr(const uint8_t* in, uint64_t size, uint8_t* out, const uint8_t* keys, const uint8_t* iv)
{
    puts("spu_aes128_decrypt_ctr()\n");

    uint64_t spu_id = 0;

    //SpuAux_Init(spu_id);

    // clear jobDone
    SPU_LS_Write64(spu_id, 0xf08, 0);
    eieio();

    {
        {
            struct Stagex_spu_params_s params;
            params.jobType = 1;

            memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x100), &params, sizeof(params));
        }

        {
            struct Stagex_spu_job_aes128_decrypt_ctr_params_s params;
            
            memcpy(params.key, keys, 16);
            memcpy(params.iv, iv, 16);

            params.in_ea = (uint64_t)in;
            params.out_ea = (uint64_t)out;

            params.size = size;

            memcpy((void*)SPU_CalcMMIOAddress_LS(spu_id, 0x200), &params, sizeof(params));
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
}

#pragma GCC pop_options