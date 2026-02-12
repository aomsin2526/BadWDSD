#include "Stagex_spu_common.c"
#include "../Aes/Aes.c"

struct __attribute__((aligned(8))) Stagex_spu_context_s
{
    uint32_t jobType; // 1 = aes128_decrypt_ctr, 2 = zlib_decompress, 3 = stage3, 4 = DecryptLv0Self, 5 = stage2
};

struct __attribute__((aligned(8))) Stagex_spu_job_aes128_decrypt_ctr_context_s
{
    uint8_t key[16];
    uint8_t iv[16];

    uint64_t in_ea;
    uint64_t out_ea;

    uint64_t size; // size to decrypt
};

void Stagex_spu_job_aes128_decrypt_ctr(const volatile struct Stagex_spu_job_aes128_decrypt_ctr_context_s *job_context, uint8_t *tmpBuf, uint32_t tmpBufSize)
{
    if ((job_context->size == 0) /*|| ((job_context->size % 16) != 0)*/)
        stop(4);

    // 0x10000 - 0x2FFFF = temp mem

    WORD aes_key[60];
    aes_key_setup((const uint8_t *)job_context->key, aes_key, 128);

    struct aes_decrypt_ctr_stream_context_s aes_ctx;
    aes_decrypt_ctr_stream_init(&aes_ctx, job_context->size, aes_key, 128, job_context->iv);

    // static const uint32_t tmpBufSize = (128 * 1024);
    // uint8_t *tmpBuf = (uint8_t *)0x10000;

    uint64_t left = job_context->size;

    uint64_t cur_in_ea = job_context->in_ea;
    uint64_t cur_out_ea = job_context->out_ea;

    while (1)
    {
        uint32_t chunkSize = (left > tmpBufSize) ? tmpBufSize : (uint32_t)left;

        DMARead(tmpBuf, cur_in_ea, chunkSize);

        aes_decrypt_ctr_stream(&aes_ctx, tmpBuf, chunkSize);

        DMAWrite(tmpBuf, cur_out_ea, chunkSize);

        cur_in_ea += chunkSize;
        cur_out_ea += chunkSize;

        left -= chunkSize;

        if (left == 0)
            break;
    }
}

#include "../uzlib/adler32.c"
#include "../uzlib/crc32.c"
#include "../uzlib/tinflate.c"
#include "../uzlib/tinfzlib.c"

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

    uint8_t *uzlib_inTmpBuf;
    uint32_t uzlib_inTmpBufSize;

    uint8_t *uzlib_outTmpBuf;
    uint32_t uzlib_outTmpBufSize;

    uint8_t *uzlib_dictTmpBuf;
    uint32_t uzlib_dictTmpBufSize;
};

int32_t Stagex_spu_job_zlib_decompress_readSource_cb(struct uzlib_uncomp *uncomp)
{
    struct Stagex_spu_job_zlib_decompress_context_s *job_context = (struct Stagex_spu_job_zlib_decompress_context_s *)uncomp->userPtr;

    uint32_t n = (job_context->uzlib_in_left < job_context->uzlib_inTmpBufSize) ? (uint32_t)job_context->uzlib_in_left : job_context->uzlib_inTmpBufSize;
    DMARead(job_context->uzlib_inTmpBuf, job_context->uzlib_cur_in_ea, n);

    job_context->uzlib_cur_in_ea += n;
    job_context->uzlib_in_left -= n;

    uncomp->source = job_context->uzlib_inTmpBuf;
    uncomp->source_limit = (uncomp->source + n);
    ++uncomp->source;

    return job_context->uzlib_inTmpBuf[0];
}

void Stagex_spu_job_zlib_decompress(volatile struct Stagex_spu_job_zlib_decompress_context_s *job_context)
{
    if (job_context->compressed_size == 0)
        stop(5);

    //

    job_context->out_decompressed_size = 0;

    //

    job_context->uzlib_inTmpBufSize = (32 * 1024);
    job_context->uzlib_inTmpBuf = (uint8_t *)0x10000;

    job_context->uzlib_outTmpBufSize = (32 * 1024);
    job_context->uzlib_outTmpBuf = (uint8_t *)0x18000;

    job_context->uzlib_dictTmpBufSize = (32 * 1024);
    job_context->uzlib_dictTmpBuf = (uint8_t *)0x20000;

    //

    job_context->uzlib_in_left = job_context->compressed_size;

    job_context->uzlib_cur_in_ea = job_context->in_ea;
    job_context->uzlib_cur_out_ea = job_context->out_ea;

    //

    struct uzlib_uncomp *uncomp = (struct uzlib_uncomp *)0x28000;
    uzlib_uncompress_init(uncomp, job_context->uzlib_dictTmpBuf, job_context->uzlib_dictTmpBufSize);

    uncomp->userPtr = job_context;

    uncomp->source = NULL;
    uncomp->source_limit = NULL;
    uncomp->source_read_cb = Stagex_spu_job_zlib_decompress_readSource_cb;

    job_context->uzlib_cur_status = uzlib_zlib_parse_header(uncomp);

    if (job_context->uzlib_cur_status < TINF_OK)
        stop(5);

    //

    while (1)
    {
        uncomp->dest_start = job_context->uzlib_outTmpBuf;
        uncomp->dest = job_context->uzlib_outTmpBuf;
        uncomp->dest_limit = (uncomp->dest + job_context->uzlib_outTmpBufSize);

        job_context->uzlib_cur_status = uzlib_uncompress(uncomp);

        {
            uint32_t n = ((uint32_t)uncomp->dest - (uint32_t)uncomp->dest_start);

            DMAWrite(job_context->uzlib_outTmpBuf, job_context->uzlib_cur_out_ea, n);
            job_context->uzlib_cur_out_ea += n;

            job_context->out_decompressed_size += n;
        }

        if (job_context->uzlib_cur_status != TINF_OK)
            break;
    }

    //

    if (job_context->uzlib_cur_status != TINF_DONE)
        stop(5);
}

void Stagex_spu_job_stage3()
{
    static const uint32_t tmpBufSize = (128 * 1024);
    uint8_t *tmpBuf = (uint8_t *)0x10000;

    uint64_t invalid_handler_addr = 0;
    uint64_t hvcallTable = 0;

    uint32_t foundInitialLPARSizeCount = 0;

    uint8_t done = 0;

    const char *lv2FilePathSearchData = "/flh/os/lv2_kernel.self";
    uint64_t lv2FilePathSearchDataSize = (strlen(lv2FilePathSearchData) + 1);

    static const uint64_t initialLparSizeSearchEndAddr = 0x200000;
    static const uint32_t foundInitialLPARSizeMaxCount = 2;

    for (uint64_t tmpBuf_CurEaAddr = 0; tmpBuf_CurEaAddr < (16 * 1024 * 1024); tmpBuf_CurEaAddr += tmpBufSize)
    {
        DMARead(tmpBuf, tmpBuf_CurEaAddr, tmpBufSize);

        for (uint32_t i = 0; i < tmpBufSize; i += 4)
        {
            uint64_t curEaAddr = (tmpBuf_CurEaAddr + i);

            if (invalid_handler_addr == 0)
            {
                const uint32_t *v = (const uint32_t *)&tmpBuf[i];

                if (v[0] == 0x38600000 && v[1] == 0x6463ffff && v[2] == 0x6063ffec && v[3] == 0x4e800020)
                    invalid_handler_addr = curEaAddr;
            }

            if ((curEaAddr < initialLparSizeSearchEndAddr) && (foundInitialLPARSizeCount < foundInitialLPARSizeMaxCount))
            {
                if (!memcmp(&tmpBuf[i], lv2FilePathSearchData, lv2FilePathSearchDataSize))
                {
#if 0

                    for (uint32_t i2 = 0; i2 < 0x200; i2 += 1)
                    {
                        if (tmpBuf[(i + i2)] != 0x18)
                            continue;

                        uint8_t newVal = 0x1B;
                        DMAWrite(&newVal, (curEaAddr + i2), 1);

                        ++foundInitialLPARSizeCount;
                    }

#else

                    if (tmpBuf[(i + 0x107)] == 0x18)
                    {
                        uint8_t newVal = 0x1B;
                        DMAWrite(&newVal, (curEaAddr + 0x107), 1);

                        ++foundInitialLPARSizeCount;
                    }

#endif
                }
            }

            if ((invalid_handler_addr != 0) && ((curEaAddr >= initialLparSizeSearchEndAddr) || (foundInitialLPARSizeCount >= foundInitialLPARSizeMaxCount)))
                done = 1;

            if (done)
                break;
        }

        if (done)
            break;
    }

    if (invalid_handler_addr == 0)
        stop(6);

    done = 0;

    for (uint64_t tmpBuf_CurEaAddr = 0; tmpBuf_CurEaAddr < (16 * 1024 * 1024); tmpBuf_CurEaAddr += tmpBufSize)
    {
        DMARead(tmpBuf, tmpBuf_CurEaAddr, tmpBufSize);

        for (uint32_t i = 0; i < tmpBufSize; i += 8)
        {
            uint64_t curEaAddr = (tmpBuf_CurEaAddr + i);

            const uint64_t *v = (const uint64_t *)&tmpBuf[i];

            if ((v[0] == invalid_handler_addr) &&
                (v[1] == invalid_handler_addr) &&
                (v[2] != invalid_handler_addr) &&
                (v[3] == invalid_handler_addr))
            {
                hvcallTable = (curEaAddr - (22 * 8));
                done = 1;
            }

            if (done)
                break;
        }

        if (done)
            break;
    }

    if (hvcallTable == 0)
        stop(6);

    {
        {
            // lv1_puts("Installing hvcall peek64(34)\n");

            uint64_t code_addr = 0x130;

            uint64_t code[1];
            code[0] = 0xE86300004E800020;

            DMAWrite(code, code_addr, sizeof(code));
            DMAWrite(&code_addr, (hvcallTable + (34 * 8)), sizeof(code_addr));
        }

        {
            // lv1_puts("Installing hvcall poke64(35)\n");

            uint64_t code_addr = 0x140;

            uint64_t code[2];
            code[0] = 0xF883000038600000;
            code[1] = 0x4E80002000000000;

            DMAWrite(code, code_addr, sizeof(code));
            DMAWrite(&code_addr, (hvcallTable + (35 * 8)), sizeof(code_addr));
        }

        {
            // lv1_puts("Installing hvcall exec(36)\n");

            uint64_t code_addr = 0x150;

            uint64_t code[6];

            code[0] = 0x3821FFF07C0802A6;
            code[1] = 0xF80100003821FF80;

            code[2] = 0x7D2903A64E800421;
            code[3] = 0x38210080E8010000;

            code[4] = 0x7C0803A638210010;
            code[5] = 0x4E80002000000000;

            DMAWrite(code, code_addr, sizeof(code));
            DMAWrite(&code_addr, (hvcallTable + (36 * 8)), sizeof(code_addr));
        }

        {
            // lv1_puts("Installing hvcall peek32(37)\n");

            uint64_t code_addr = 0x180;

            uint64_t code[1];
            code[0] = 0x806300004E800020;

            DMAWrite(code, code_addr, sizeof(code));
            DMAWrite(&code_addr, (hvcallTable + (37 * 8)), sizeof(code_addr));
        }

        {
            // lv1_puts("Installing hvcall poke32(38)\n");

            uint64_t code_addr = 0x190;

            uint64_t code[2];
            code[0] = 0x9083000038600000;
            code[1] = 0x4E80002000000000;

            DMAWrite(code, code_addr, sizeof(code));
            DMAWrite(&code_addr, (hvcallTable + (38 * 8)), sizeof(code_addr));
        }
    }
}

struct ElfHeader_s
{
    uint8_t e_ident[16];  /* ELF identification */
    uint16_t e_type;      /* object file type */
    uint16_t e_machine;   /* machine type */
    uint32_t e_version;   /* object file version */
    uint64_t e_entry;     /* entry point address */
    uint64_t e_phoff;     /* program header offset */
    uint64_t e_shoff;     /* section header offset */
    uint32_t e_flags;     /* processor-specific flags */
    uint16_t e_ehsize;    /* ELF header size */
    uint16_t e_phentsize; /* size of program header entry */
    uint16_t e_phnum;     /* number of program header entries */
    uint16_t e_shentsize; /* size of section header entry */
    uint16_t e_shnum;     /* number of section header entries */
    uint16_t e_shstrndx;  /* section name string table index */
} __attribute__((packed));

struct ElfPhdr_s
{
    uint32_t p_type;   /* Segment type */
    uint32_t p_flags;  /* Segment flags */
    uint64_t p_offset; /* Segment file offset */
    uint64_t p_vaddr;  /* Segment virtual address */
    uint64_t p_paddr;  /* Segment physical address */
    uint64_t p_filesz; /* Segment size in file */
    uint64_t p_memsz;  /* Segment size in memory */
    uint64_t p_align;  /* Segment alignment */
};

struct SceHeader_s
{
    uint32_t magic;
    uint32_t version;
    uint16_t attribute;
    uint16_t category;
    uint32_t ext_header_size;
    uint64_t file_offset;
    uint64_t file_size;
};

struct SceMetaInfo_s
{
    uint64_t key[2];
    uint8_t key_pad[16];
    uint64_t iv[2];
    uint8_t iv_pad[16];
};

struct SceMetaHeader_s
{
    uint64_t sign_offset;
    uint32_t sign_algorithm; // 1 = ECDSA160, 2 = HMACSHA1, 3 = SHA1, 5 = RSA2048, 6 = HMACSHA256 (?not used?)
    uint32_t section_entry_num;
    uint32_t key_entry_num;
    uint32_t optional_header_size;
    uint64_t pad;
};

struct SceMetaSectionHeader_s
{
    uint64_t segment_offset;
    uint64_t segment_size;
    uint32_t segment_type;   // 1 = shdr, 2 = phdr, 3 = sceversion
    uint32_t segment_id;     // 0,1,2,3,etc for phdr, always 3 for shdrs, sceversion shdr number for sceversion
    uint32_t sign_algorithm; // 1 = ECDSA160 (not used), 2 = HMACSHA1, 3 = SHA1, 5 = RSA2048 (not used), 6 = HMACSHA256
    uint32_t sign_idx;
    uint32_t enc_algorithm;  // 1 = none, 2 = aes128cbccfb, 3 = aes128ctr
    uint32_t key_idx;        // -1 when enc_algorithm = none
    uint32_t iv_idx;         // -1 when enc_algorithm = none
    uint32_t comp_algorithm; // 1 = plain, 2 = zlib
};

struct SceMetaKey_s
{
    uint64_t key[2];
};

struct __attribute__((aligned(8))) Stagex_spu_job_DecryptLv0Self_context_s
{
    uint64_t inDestEa;
    uint64_t inSrcEa;
};

void Stagex_spu_job_DecryptLv0Self(const volatile struct Stagex_spu_job_DecryptLv0Self_context_s *job_context)
{
    uint64_t curDestEa = job_context->inDestEa;
    uint64_t curSrcEa = job_context->inSrcEa;

    struct SceHeader_s sceHeader;
    DMARead(&sceHeader, curSrcEa, sizeof(sceHeader));

    if ((sceHeader.magic) != 0x53434500)
        stop(7);

    uint64_t meta_key[4];
    meta_key[0] = (0xCA7A24EC38BDB45B);
    meta_key[1] = (0x98CCD7D363EA2AF0);
    meta_key[2] = (0xC326E65081E0630C);
    meta_key[3] = (0xB9AB2D215865878A);

    uint64_t meta_iv[2];
    meta_iv[0] = (0xF9205F46F6021697);
    meta_iv[1] = (0xE670F13DFA726212);

    WORD meta_aes_key[60];
    aes_key_setup((const uint8_t *)meta_key, meta_aes_key, 256);

    curSrcEa += 0x200;

    struct SceMetaInfo_s metaInfo;
    DMARead(&metaInfo, curSrcEa, sizeof(metaInfo));

    aes_decrypt_cbc(
        (const uint8_t *)&metaInfo,
        sizeof(metaInfo),

        (uint8_t *)&metaInfo,

        meta_aes_key,
        256,

        (const uint8_t *)meta_iv);

    curSrcEa += sizeof(metaInfo);

    WORD meta_header_key[60];
    aes_key_setup((const uint8_t *)metaInfo.key, meta_header_key, 128);

    uint64_t metasSize = (sceHeader.file_offset) - sizeof(struct SceHeader_s) + (sceHeader.ext_header_size) + sizeof(struct SceMetaInfo_s);
    uint8_t *metasBuf = (uint8_t *)0x20000; // [16384]
    DMARead(metasBuf, curSrcEa, metasSize);

    aes_decrypt_ctr(

        metasBuf,
        metasSize,

        metasBuf,

        meta_header_key,
        128,

        (const uint8_t *)metaInfo.iv

    );

    const struct SceMetaHeader_s *metaHeader = (const struct SceMetaHeader_s *)&metasBuf[0];
    const struct SceMetaSectionHeader_s *metaSectionHeaders = (const struct SceMetaSectionHeader_s *)&metasBuf[sizeof(struct SceMetaHeader_s)];
    const struct SceMetaKey_s *metaKeys = (const struct SceMetaKey_s *)&metasBuf[sizeof(struct SceMetaHeader_s) + ((metaHeader->section_entry_num) * sizeof(struct SceMetaSectionHeader_s))];

    uint8_t *tmpBuf = (uint8_t *)0x24000; // [16384]

    struct ElfHeader_s elfHeader;
    DMARead(&elfHeader, (job_context->inSrcEa + 0x90), sizeof(elfHeader));
    DMAWrite(&elfHeader, curDestEa, sizeof(elfHeader));

    {
        uint32_t copySize = (elfHeader.e_phentsize * elfHeader.e_phnum);

        curDestEa += elfHeader.e_phoff;
        DMARead(tmpBuf, (job_context->inSrcEa + 0x90 + elfHeader.e_phoff), copySize);
        DMAWrite(tmpBuf, curDestEa, copySize);
    }

    const struct ElfPhdr_s *elfPhdrs = (const struct ElfPhdr_s *)tmpBuf;

    for (uint16_t i = 0; i < elfHeader.e_phnum; ++i)
    {
        const struct ElfPhdr_s *phdr = &elfPhdrs[i];

        const struct SceMetaSectionHeader_s *h = &metaSectionHeaders[i];

        const struct SceMetaKey_s *key = &metaKeys[h->key_idx];
        const struct SceMetaKey_s *iv = &metaKeys[h->iv_idx];

        uint64_t in_addr = (job_context->inSrcEa + h->segment_offset);
        uint64_t out_addr = (job_context->inDestEa + phdr->p_offset);

        volatile struct Stagex_spu_job_aes128_decrypt_ctr_context_s decrypt;

        memcpy(decrypt.key, key->key, 16);
        memcpy(decrypt.iv, iv->key, 16);

        decrypt.in_ea = in_addr;
        decrypt.out_ea = out_addr;

        decrypt.size = h->segment_size;

        Stagex_spu_job_aes128_decrypt_ctr(&decrypt, (uint8_t *)0x10000, (64 * 1024));
    }
}

struct __attribute__((aligned(8))) Stagex_spu_job_stage2_context_s
{
    uint8_t patch_aim;
    uint8_t patch_inspect_package_tophalf;
};

void Stagex_spu_job_stage2(const volatile struct Stagex_spu_job_stage2_context_s *job_context)
{
    static const uint32_t tmpBufSize = (128 * 1024);
    uint8_t *tmpBuf = (uint8_t *)0x10000;

    uint8_t found_CoreOSHashCheck = 0;

    uint8_t found_disable_erase_hash_standby_bank_and_fsm = 0;
    uint8_t found_get_version_and_hash = 0;

    uint8_t found_hvcall_114_1 = 0;
    uint8_t found_hvcall_114_2 = 0;

    uint8_t found_fsm = 0;

    uint8_t found_aim_get_device_type = 0;
    uint8_t found_aim_get_device_id = 0;

    uint8_t found_aim_get_ps_code = 0;

    uint8_t found_inspect_package_tophalf = 0;

    for (uint64_t tmpBuf_CurEaAddr = 0; tmpBuf_CurEaAddr < (8 * 1024 * 1024); tmpBuf_CurEaAddr += tmpBufSize)
    {
        DMARead(tmpBuf, tmpBuf_CurEaAddr, tmpBufSize);

        for (uint32_t i = 0; i < tmpBufSize; i += 4)
        {
            uint64_t curEaAddr = (tmpBuf_CurEaAddr + i);

            if (!found_CoreOSHashCheck)
            {
                // puts("Patching CoreOS hash check...\n");

                __attribute__((aligned(16))) static const uint8_t searchData[] = {0x88, 0x18, 0x00, 0x36, 0x2F, 0x80, 0x00, 0xFF, 0x41, 0x9E, 0x00, 0x1C, 0x7F, 0x63, 0xDB, 0x78, 0xE8, 0xA2, 0x85, 0x78};
                __attribute__((aligned(16))) static const uint8_t replaceData[] = {0x88, 0x18, 0x00, 0x36, 0x2F, 0x80, 0x00, 0xFF, 0x60, 0x00, 0x00, 0x00, 0x7F, 0x63, 0xDB, 0x78, 0xE8, 0xA2, 0x85, 0x78};

                if (!memcmp32(&tmpBuf[i], searchData, sizeof(searchData)))
                {
                    DMAWrite(replaceData, curEaAddr, sizeof(replaceData));
                    found_CoreOSHashCheck = 1;
                }
            }

            if (!found_disable_erase_hash_standby_bank_and_fsm)
            {
                // puts("Patching disable_erase_hash_standby_bank_and_fsm (ANTI BRICK & EXIT FSM)...\n");

                __attribute__((aligned(16))) static const uint8_t searchData[] = {0xF8, 0x21, 0xFE, 0xC1, 0x7C, 0x08, 0x02, 0xA6, 0xFB, 0x41, 0x01, 0x10, 0x3B, 0x41, 0x00, 0x70, 0xFB, 0xA1, 0x01, 0x28, 0x7C, 0x7D, 0x1B, 0x78, 0x7F, 0x43, 0xD3, 0x78};
                __attribute__((aligned(16))) static const uint8_t replaceData[] = {0x7C, 0x08, 0x02, 0xA6, 0x38, 0x21, 0xFF, 0xC0, 0xF8, 0x01, 0x00, 0x00, 0x3D, 0x20, 0x80, 0x00, 0x61, 0x29, 0x41, 0x24, 0x79, 0x29, 0x00, 0x20, 0x38, 0x60, 0x00, 0xFF, 0x38, 0x80, 0x00, 0xFF, 0x7D, 0x29, 0x03, 0xA6, 0x4E, 0x80, 0x04, 0x21, 0xE8, 0x01, 0x00, 0x00, 0x38, 0x21, 0x00, 0x40, 0x38, 0x60, 0x00, 0x00, 0x7C, 0x08, 0x03, 0xA6, 0x4E, 0x80, 0x00, 0x20};

                if (!memcmp32(&tmpBuf[i], searchData, sizeof(searchData)))
                {
                    DMAWrite(replaceData, curEaAddr, sizeof(replaceData));
                    found_disable_erase_hash_standby_bank_and_fsm = 1;
                }
            }

            if (!found_get_version_and_hash)
            {
                // puts("Patching get_version_and_hash (ANTI BRICK & Downgrading)...\n");

                __attribute__((aligned(16))) static const uint8_t searchData[] = {0x4B, 0xFF, 0x45, 0xD1, 0xE8, 0x1F, 0x00, 0x80, 0xF8, 0x1C, 0x00, 0x00, 0x38, 0x9D, 0x00, 0x08};
                __attribute__((aligned(16))) static const uint8_t replaceData[] = {0x38, 0x00, 0x40, 0x08, 0x78, 0x00, 0x26, 0xC6, 0xF8, 0x1C, 0x00, 0x00, 0x38, 0x9D, 0x00, 0x08};

                if (!memcmp32(&tmpBuf[i], searchData, sizeof(searchData)))
                {
                    DMAWrite(replaceData, curEaAddr, sizeof(replaceData));
                    found_get_version_and_hash = 1;
                }
            }

            if (!found_hvcall_114_1)
            {
                // puts("Patching hvcall 114 1...\n");

                __attribute__((aligned(16))) static const uint8_t searchData[] = {0x2F, 0x80, 0x00, 0x00, 0x41, 0x9E, 0x00, 0x28, 0x38, 0x60, 0x00, 0x00, 0x38, 0x80, 0x00, 0x00};
                __attribute__((aligned(16))) static const uint8_t replaceData[] = {0x60, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x28, 0x38, 0x60, 0x00, 0x00, 0x38, 0x80, 0x00, 0x00};

                if (!memcmp32(&tmpBuf[i], searchData, sizeof(searchData)))
                {
                    DMAWrite(replaceData, curEaAddr, sizeof(replaceData));
                    found_hvcall_114_1 = 1;
                }
            }

            if (!found_hvcall_114_2)
            {
                // puts("Patching hvcall 114 2...\n");

                __attribute__((aligned(16))) static const uint8_t searchData[] = {0x7C, 0x08, 0x03, 0x78, 0x39, 0x20, 0x00, 0x00, 0x4B, 0xFF, 0xFB, 0xFD, 0x7C, 0x60, 0x1B, 0x78};
                __attribute__((aligned(16))) static const uint8_t replaceData[] = {0x7C, 0x08, 0x03, 0x78, 0x39, 0x20, 0x00, 0x01, 0x4B, 0xFF, 0xFB, 0xFD, 0x7C, 0x60, 0x1B, 0x78};

                if (!memcmp32(&tmpBuf[i], searchData, sizeof(searchData)))
                {
                    DMAWrite(replaceData, curEaAddr, sizeof(replaceData));
                    found_hvcall_114_2 = 1;
                }
            }

            if (!found_fsm)
            {
                // puts("Patching FSM...\n");

                __attribute__((aligned(16))) static const uint8_t searchData[] = {0x80, 0x01, 0x00, 0x74, 0x7F, 0xC3, 0xF3, 0x78, 0xE8, 0xA2, 0x84, 0xE8, 0x38, 0x80, 0x00, 0x01};
                __attribute__((aligned(16))) static const uint8_t replaceData[] = {0x38, 0x00, 0x00, 0xFF, 0x7F, 0xC3, 0xF3, 0x78, 0xE8, 0xA2, 0x84, 0xE8, 0x38, 0x80, 0x00, 0x01};

                if (!memcmp32(&tmpBuf[i], searchData, sizeof(searchData)))
                {
                    DMAWrite(replaceData, curEaAddr, sizeof(replaceData));
                    found_fsm = 1;
                }
            }

            if (job_context->patch_aim)
            {
                if (!found_aim_get_device_type)
                {
                    __attribute__((aligned(16))) static const uint8_t searchData[] = {0xF8, 0x21, 0xFF, 0x61, 0x7C, 0x08, 0x02, 0xA6, 0xF8, 0x81, 0x00, 0x70, 0xF8, 0x01, 0x00, 0xB0, 0x38, 0x00, 0x00, 0x00, 0x38, 0xA1, 0x00, 0x78, 0x38, 0x81, 0x00, 0x80, 0x38, 0xC1, 0x00, 0x70, 0xF8, 0x01, 0x00, 0x80, 0x38, 0x00, 0x00, 0x10, 0xFB, 0xE1, 0x00, 0x98, 0xF8, 0x01, 0x00, 0x78, 0x48, 0x00, 0x02, 0xF1, 0xE8, 0xA2, 0x82, 0x80, 0x7C, 0x7F, 0x1B, 0x78, 0xE8, 0x62, 0x82, 0x60};
                    __attribute__((aligned(16))) static const uint8_t replaceData[] = {0x38, 0xC0, 0x00, 0x00, 0xB0, 0xC4, 0x00, 0x00, 0x38, 0xC0, 0x00, 0x00, 0xB0, 0xC4, 0x00, 0x02, 0x38, 0xC0, 0x00, 0x00, 0xB0, 0xC4, 0x00, 0x04, 0x38, 0xC0, 0x00, 0x00, 0xB0, 0xC4, 0x00, 0x06, 0x38, 0xC0, 0x00, 0x00, 0xB0, 0xC4, 0x00, 0x08, 0x38, 0xC0, 0x00, 0x00, 0xB0, 0xC4, 0x00, 0x0A, 0x38, 0xC0, 0x00, 0x00, 0xB0, 0xC4, 0x00, 0x0C, 0x38, 0xC0, 0x00, 0x82, 0xB0, 0xC4, 0x00, 0x0E, 0x38, 0x60, 0x00, 0x00, 0x4E, 0x80, 0x00, 0x20};

                    if (!memcmp32(&tmpBuf[i], searchData, sizeof(searchData)))
                    {
                        DMAWrite(replaceData, curEaAddr, sizeof(replaceData));
                        found_aim_get_device_type = 1;
                    }
                }

                if (!found_aim_get_device_id)
                {
                    __attribute__((aligned(16))) static const uint8_t searchData[] = {0xF8, 0x21, 0xFF, 0x61, 0x7C, 0x08, 0x02, 0xA6, 0xF8, 0x81, 0x00, 0x70, 0xF8, 0x01, 0x00, 0xB0, 0x38, 0x00, 0x00, 0x00, 0x38, 0xA1, 0x00, 0x78, 0x38, 0x81, 0x00, 0x80, 0x38, 0xC1, 0x00, 0x70, 0xF8, 0x01, 0x00, 0x80, 0x38, 0x00, 0x00, 0x10, 0xFB, 0xE1, 0x00, 0x98, 0xF8, 0x01, 0x00, 0x78, 0x48, 0x00, 0x03, 0x61, 0xE8, 0xA2, 0x82, 0x78, 0x7C, 0x7F, 0x1B, 0x78, 0xE8, 0x62, 0x82, 0x60};
                    __attribute__((aligned(16))) static const uint8_t replaceData[] = {0x38, 0xC0, 0x00, 0x00, 0xB0, 0xC4, 0x00, 0x00, 0x38, 0xC0, 0x00, 0x01, 0xB0, 0xC4, 0x00, 0x02, 0x38, 0xC0, 0x00, 0x82, 0xB0, 0xC4, 0x00, 0x04, 0x38, 0xC0, 0x00, 0x01, 0xB0, 0xC4, 0x00, 0x06, 0x38, 0xC0, 0x04, 0x00, 0xB0, 0xC4, 0x00, 0x08, 0x38, 0xC0, 0x23, 0xBB, 0xB0, 0xC4, 0x00, 0x0A, 0x38, 0xC0, 0x5E, 0xDF, 0xB0, 0xC4, 0x00, 0x0C, 0x38, 0xC0, 0x37, 0x05, 0xB0, 0xC4, 0x00, 0x0E, 0x38, 0x60, 0x00, 0x00, 0x4E, 0x80, 0x00, 0x20};

                    if (!memcmp32(&tmpBuf[i], searchData, sizeof(searchData)))
                    {
                        DMAWrite(replaceData, curEaAddr, sizeof(replaceData));
                        found_aim_get_device_id = 1;
                    }
                }

                if (!found_aim_get_ps_code)
                {
                    __attribute__((aligned(16))) static const uint8_t searchData[] = {0xF8, 0x21, 0xFF, 0x61, 0x7C, 0x08, 0x02, 0xA6, 0xF8, 0x81, 0x00, 0x70, 0xF8, 0x01, 0x00, 0xB0, 0x38, 0x00, 0x00, 0x00, 0x38, 0xA1, 0x00, 0x78, 0x38, 0x81, 0x00, 0x80, 0x38, 0xC1, 0x00, 0x70, 0xF8, 0x01, 0x00, 0x80, 0x38, 0x00, 0x00, 0x08, 0xFB, 0xE1, 0x00, 0x98, 0xF8, 0x01, 0x00, 0x78, 0x48, 0x00, 0x03, 0xD1, 0xE8, 0xA2, 0x82, 0x70, 0x7C, 0x7F, 0x1B, 0x78, 0xE8, 0x62, 0x82, 0x60};
                    __attribute__((aligned(16))) static const uint8_t replaceData[] = {0x38, 0xC0, 0x00, 0x01, 0xB0, 0xC4, 0x00, 0x00, 0x38, 0xC0, 0x00, 0x82, 0xB0, 0xC4, 0x00, 0x02, 0x38, 0xC0, 0x00, 0x01, 0xB0, 0xC4, 0x00, 0x04, 0x38, 0xC0, 0x00, 0x01, 0xB0, 0xC4, 0x00, 0x06, 0x38, 0x60, 0x00, 0x00, 0x4E, 0x80, 0x00, 0x20};

                    if (!memcmp32(&tmpBuf[i], searchData, sizeof(searchData)))
                    {
                        DMAWrite(replaceData, curEaAddr, sizeof(replaceData));
                        found_aim_get_ps_code = 1;
                    }
                }
            }

            if (job_context->patch_inspect_package_tophalf)
            {
                if (!found_inspect_package_tophalf)
                {
                    __attribute__((aligned(16))) static const uint8_t searchData[] = {0x41, 0x9D, 0x00, 0xA8, 0x7B, 0xFD, 0x00, 0x20, 0x7F, 0x44, 0xD3, 0x78, 0x7F, 0xA5, 0xEB, 0x78, 0x7F, 0xC6, 0xF3, 0x78, 0x7E, 0xA3, 0xAB, 0x78, 0x40, 0x9A, 0x00, 0x90, 0x4B, 0xFF, 0xFD, 0x8D};
                    __attribute__((aligned(16))) static const uint8_t replaceData[] = {0x40, 0x9D, 0x00, 0xA8, 0x7B, 0xFD, 0x00, 0x20, 0x7F, 0x44, 0xD3, 0x78, 0x7F, 0xA5, 0xEB, 0x78, 0x7F, 0xC6, 0xF3, 0x78, 0x7E, 0xA3, 0xAB, 0x78, 0x41, 0x9A, 0x00, 0x90, 0x4B, 0xFF, 0xFD, 0x8D};

                    if (!memcmp32(&tmpBuf[i], searchData, sizeof(searchData)))
                    {
                        DMAWrite(replaceData, curEaAddr, sizeof(replaceData));
                        found_inspect_package_tophalf = 1;
                    }
                }
            }
        }
    }

    if (!found_disable_erase_hash_standby_bank_and_fsm || !found_get_version_and_hash)
        stop(8);

    if (job_context->patch_inspect_package_tophalf)
    {
        if (!found_inspect_package_tophalf)
            stop(8);
    }
}

void main()
{
    volatile uint64_t *jobStart = (volatile uint64_t *)0xf00;
    volatile uint64_t *jobDone = (volatile uint64_t *)0xf08;

    *jobStart = 0;
    *jobDone = 0;

    sync();

    volatile uint64_t *spuReady = (volatile uint64_t *)0xf10;
    *spuReady = 1;

    sync();

    while (1)
    {
        if (*jobStart != 0)
        {
            *jobStart = 0;
            sync();

            const volatile struct Stagex_spu_context_s *context = (const volatile struct Stagex_spu_context_s *)0x100;

            if (context->jobType == 1)
                Stagex_spu_job_aes128_decrypt_ctr((const volatile struct Stagex_spu_job_aes128_decrypt_ctr_context_s *)0x200, (uint8_t *)0x10000, (128 * 1024));
            else if (context->jobType == 2)
                Stagex_spu_job_zlib_decompress((volatile struct Stagex_spu_job_zlib_decompress_context_s *)0x200);
            else if (context->jobType == 3)
                Stagex_spu_job_stage3();
            else if (context->jobType == 4)
                Stagex_spu_job_DecryptLv0Self((const volatile struct Stagex_spu_job_DecryptLv0Self_context_s *)0x200);
            else if (context->jobType == 5)
                Stagex_spu_job_stage2((const volatile struct Stagex_spu_job_stage2_context_s *)0x200);
            else
                stop(3);

            sync();
            *jobDone = 1;
            sync();
        }
    }

    stop(99);
}