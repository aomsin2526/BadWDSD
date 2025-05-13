#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/stat.h>

#define round_up(x, n) (-(-(x) & -(n)))

size_t get_file_size(FILE *f)
{
    size_t old_off = ftell(f);

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);

    fseek(f, old_off, SEEK_SET);
    return size;
}

uint16_t endswap16(uint16_t v)
{
    return __bswap_16(v);
}

uint32_t endswap32(uint32_t v)
{
    return __bswap_32(v);
}

uint64_t endswap64(uint64_t v)
{
    return __bswap_64(v);
}

void lv2diff(const char *inFilePath1, const char *inFilePath2, const char *outFilePath)
{
    printf("lv2diff()\n");

    printf("inFilePath1 = %s\n", inFilePath1);
    printf("inFilePath2 = %s\n", inFilePath2);
    printf("outFilePath = %s\n", outFilePath);

    FILE *inFile1 = fopen(inFilePath1, "rb");
    FILE *inFile2 = fopen(inFilePath2, "rb");
    FILE *outFile = fopen(outFilePath, "wb");

    if (inFile1 == NULL || inFile2 == NULL || outFile == NULL)
    {
        printf("open file failed!\n");

        abort();
        return;
    }

    size_t inFileSize1 = get_file_size(inFile1);
    printf("inFileSize1 = %lu\n", inFileSize1);

    uint8_t *inData1 = (uint8_t *)malloc(inFileSize1);

    if (inData1 == NULL)
    {
        printf("malloc failed!\n");

        abort();
        return;
    }

    fread(inData1, 1, inFileSize1, inFile1);
    fclose(inFile1);

    size_t inFileSize2 = get_file_size(inFile2);
    printf("inFileSize2 = %lu\n", inFileSize2);

    uint8_t *inData2 = (uint8_t *)malloc(inFileSize2);

    if (inData2 == NULL)
    {
        printf("malloc failed!\n");

        abort();
        return;
    }

    fread(inData2, 1, inFileSize2, inFile2);
    fclose(inFile2);

    if (inFileSize1 != inFileSize2)
    {
        printf("file size must be equal!\n");

        abort();
        return;
    }

    size_t sz = inFileSize1;
    uint32_t diffCount = 0;

    for (size_t i = 0x10000; i < sz; ++i)
    {
        if (inData1[i] != inData2[i])
            ++diffCount;
    }

    printf("diffCount = %u\n", diffCount);

    diffCount = endswap32(diffCount);
    fwrite(&diffCount, 4, 1, outFile);
    diffCount = endswap32(diffCount);

    uint32_t diffCount2 = 0;

    for (size_t i = 0x10000; i < sz; ++i)
    {
        if (inData1[i] != inData2[i])
        {
            uint32_t addr = (uint32_t)(i);

            uint32_t val = 0;

            val |= (uint8_t)inData2[i]; // new
            val <<= 8;
            val |= (uint8_t)inData1[i]; // orig

            // shitty code

            if (addr >= 0x10000)
                addr -= 0x10000;
            else
                continue;

            addr = endswap32(addr);
            val = endswap32(val);

            fwrite(&addr, 4, 1, outFile);
            fwrite(&val, 4, 1, outFile);

            ++diffCount2;
        }
    }

    if (diffCount != diffCount2)
    {
        printf("diffCount must be equal!\n");

        abort();
        return;
    }

    free(inData2);
    free(inData1);

    fclose(outFile);
}

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

#include "../Aes/Aes.h"
#include "../Aes/Aes.c"

#include "../tinf/tinf.h"

#include "../tinf/adler32.c"
#include "../tinf/crc32.c"
#include "../tinf/tinflate.c"
#include "../tinf/tinfzlib.c"

void DecryptLv2Self(void *inDest, const void *inSrc)
{
    printf("DecryptLv2Self()\n");

    uint8_t *dest = (uint8_t *)inDest;
    const uint8_t *src = (const uint8_t *)inSrc;

    uint64_t curSrcOffset = 0;

    struct SceHeader_s sceHeader;
    memcpy(&sceHeader, &src[curSrcOffset], sizeof(struct SceHeader_s));

    if (endswap32(sceHeader.magic) != 0x53434500)
    {
        printf("SCE magic check failed!, 0x%x\n", endswap32(sceHeader.magic));

        abort();
        return;
    }

    printf("sceHeader:\n");
    printf("ext_header_size = 0x%x\n", endswap32(sceHeader.ext_header_size));
    printf("file_offset = 0x%lx\n", endswap64(sceHeader.file_offset));
    printf("file_size = 0x%lx\n", endswap64(sceHeader.file_size));

    // erk=0CAF212B6FA53C0D A7E2C575ADF61DBE 68F34A33433B1B89 1ABF5C4251406A03
    // riv=9B79374722AD888E B6A35A2DF25A8B3E

    uint64_t meta_key[4];
    meta_key[0] = endswap64(0x0CAF212B6FA53C0D);
    meta_key[1] = endswap64(0xA7E2C575ADF61DBE);
    meta_key[2] = endswap64(0x68F34A33433B1B89);
    meta_key[3] = endswap64(0x1ABF5C4251406A03);

    uint64_t meta_iv[2];
    meta_iv[0] = endswap64(0x9B79374722AD888E);
    meta_iv[1] = endswap64(0xB6A35A2DF25A8B3E);

    WORD meta_aes_key[60];
    aes_key_setup((const uint8_t *)meta_key, meta_aes_key, 256);

    curSrcOffset = 0x200;

    SceMetaInfo_s metaInfo;
    aes_decrypt_cbc(

        &src[curSrcOffset],
        sizeof(struct SceMetaInfo_s),

        (uint8_t *)&metaInfo,

        meta_aes_key,
        256,

        (const uint8_t *)meta_iv);

    curSrcOffset += sizeof(struct SceMetaInfo_s);

    printf("metaInfo:\n");
    printf("metaInfo.key[0] = 0x%lx\n", endswap64(metaInfo.key[0]));
    printf("metaInfo.key[1] = 0x%lx\n", endswap64(metaInfo.key[1]));

    printf("metaInfo.iv[0] = 0x%lx\n", endswap64(metaInfo.iv[0]));
    printf("metaInfo.iv[1] = 0x%lx\n", endswap64(metaInfo.iv[1]));

    WORD meta_header_key[60];
    aes_key_setup((const uint8_t *)metaInfo.key, meta_header_key, 128);

    uint64_t metasSize = endswap64(sceHeader.file_offset) - sizeof(struct SceHeader_s) + endswap32(sceHeader.ext_header_size) + sizeof(struct SceMetaInfo_s);
    printf("metasSize = %lu\n", metasSize);

    uint8_t metasBuf[16384];

    aes_decrypt_ctr(

        &src[curSrcOffset],
        metasSize,

        metasBuf,

        meta_header_key,
        128,

        (const uint8_t *)metaInfo.iv

    );

    SceMetaHeader_s *metaHeader = (SceMetaHeader_s *)&metasBuf[0];

    printf("metaHeader:\n");
    printf("section_entry_num = %u\n", endswap32(metaHeader->section_entry_num));
    printf("key_entry_num = %u\n", endswap32(metaHeader->key_entry_num));

    SceMetaSectionHeader_s *metaSectionHeaders = (SceMetaSectionHeader_s *)&metasBuf[sizeof(struct SceMetaHeader_s)];

    for (uint32_t i = 0; i < endswap32(metaHeader->section_entry_num); ++i)
    {
        SceMetaSectionHeader_s *h = &metaSectionHeaders[i];

        printf("section_headers[%u]:\n", i);
        printf("segment_id = %u, segment_offset = 0x%lx, segment_size = 0x%lx, enc_algorithm = %u, key_idx = %u, iv_idx = %u\n",
               endswap32(h->segment_id), endswap64(h->segment_offset), endswap64(h->segment_size), endswap32(h->enc_algorithm), endswap32(h->key_idx), endswap32(h->iv_idx));
    }

    SceMetaKey_s *metaKeys = (SceMetaKey_s *)&metasBuf[sizeof(struct SceMetaHeader_s) + (endswap32(metaHeader->section_entry_num) * sizeof(struct SceMetaSectionHeader_s))];

    for (uint32_t i = 0; i < endswap32(metaHeader->key_entry_num); ++i)
    {
        SceMetaKey_s *k = &metaKeys[i];

        printf("keys[%u]: ", i);

        printf("key[0] = 0x%lx", endswap64(k->key[0]));
        printf(", key[1] = 0x%lx\n", endswap64(k->key[1]));
    }

    ElfHeader_s *elfHeader = (ElfHeader_s *)&src[0x90];

    memcpy(dest, elfHeader, sizeof(ElfHeader_s));
    memcpy(dest + endswap64(elfHeader->e_phoff), &src[0x90 + endswap64(elfHeader->e_phoff)], endswap16(elfHeader->e_phentsize) * endswap16(elfHeader->e_phnum));

    ElfPhdr_s *elfPhdrs = (ElfPhdr_s *)(dest + endswap64(elfHeader->e_phoff));

    uint8_t *decryptBuf = (uint8_t *)malloc(16 * 1024 * 1024);

    for (uint16_t i = 0; i < endswap16(elfHeader->e_phnum); ++i)
    {
        ElfPhdr_s *phdr = &elfPhdrs[i];

        printf("decrypting phdr %u...\n", (uint32_t)i);

        SceMetaSectionHeader_s *h = &metaSectionHeaders[i];

        SceMetaKey_s *key = &metaKeys[endswap32(h->key_idx)];
        SceMetaKey_s *iv = &metaKeys[endswap32(h->iv_idx)];

        WORD aes_key[60];
        aes_key_setup((const uint8_t *)key->key, aes_key, 128);

        aes_decrypt_ctr(

            &src[endswap64(h->segment_offset)],
            endswap64(h->segment_size),

            decryptBuf,

            aes_key,
            128,

            (const uint8_t *)iv->key

        );

        printf("decompressing...\n");

        uint32_t sz = endswap64(phdr->p_filesz);

        int32_t res = tinf_zlib_uncompress(
            dest + endswap64(phdr->p_offset), &sz, 
            decryptBuf, (uint32_t)endswap64(h->segment_size)
        );

        if (res != TINF_OK || sz != endswap64(phdr->p_filesz))
        {
            printf("Decompress failed!, sz = %u, p_filesz = %lu\n", sz, endswap64(phdr->p_filesz));
            
            abort();
            return;
        }
    }

    free(decryptBuf);
}

void lv2decrypt(const char *inFilePath, const char *outFilePath)
{
    printf("lv2decrypt()\n");

    printf("inFilePath = %s\n", inFilePath);
    printf("outFilePath = %s\n", outFilePath);

    FILE *inFile = fopen(inFilePath, "rb");
    FILE *outFile = fopen(outFilePath, "wb");

    if (inFile == NULL || outFile == NULL)
    {
        printf("open file failed!\n");

        abort();
        return;
    }

    size_t inFileSize = get_file_size(inFile);
    printf("inFileSize = %lu\n", inFileSize);

    uint8_t *inData = (uint8_t *)malloc(inFileSize);

    if (inData == NULL)
    {
        printf("malloc failed!\n");

        abort();
        return;
    }

    fread(inData, 1, inFileSize, inFile);
    fclose(inFile);

    size_t outFileSize = (inFileSize * 8);
    uint8_t *outData = (uint8_t *)malloc(outFileSize);

    if (outData == NULL)
    {
        printf("malloc failed!\n");

        abort();
        return;
    }

    DecryptLv2Self(outData, inData);

    fwrite(outData, 1, outFileSize, outFile);
    fclose(outFile);

    free(outData);
    free(inData);
}

#include "hmac/sha1.h"
#include "hmac/sha1.c"
#include "hmac/hmac.c"

void lv2hash(const char* inFilePath)
{
    printf("lv2hash()\n");

    printf("inFilePath = %s\n", inFilePath);

    FILE *inFile = fopen(inFilePath, "rb");

    if (inFile == NULL)
    {
        printf("open file failed!\n");

        abort();
        return;
    }

    size_t inFileSize = get_file_size(inFile);
    printf("inFileSize = %lu\n", inFileSize);

    uint8_t *inData = (uint8_t *)malloc(inFileSize);

    if (inData == NULL)
    {
        printf("malloc failed!\n");

        abort();
        return;
    }

    fread(inData, 1, inFileSize, inFile);
    fclose(inFile);

    //void hmac_sha1(const uint8_t *text, size_t text_len, const uint8_t *key, size_t key_len, uint8_t* digest)
    
    uint8_t key[] = {0xA0, 0x9B, 0x58, 0xA6, 0x12, 0xB9, 0xF4, 0xC1, 0x34, 0x51, 0xA1, 
        0xB8, 0x1C, 0x94, 0xAB, 0xF8, 0x42, 0x3E, 0xD7, 0x6A, 0x96, 0x27, 0x1A, 0x72, 
        0x23, 0x94, 0xF0, 0xDD, 0x04, 0x2B, 0xA2, 0xCA, 0xA4, 0x1A, 0x56, 0x71, 0x77, 
        0xA8, 0xB5, 0x00, 0x23, 0x5C, 0x74, 0x49, 0x58, 0x42, 0xBF, 0x20, 0x07, 0xFA, 
        0xF2, 0x74, 0xCC, 0x81, 0x09, 0x1A, 0xD5, 0x7A, 0xF7, 0x26, 0x4A, 0x60, 0xE2, 0xCE};

    uint8_t digest[24];
    uint64_t* digest64 = (uint64_t*)digest;
    digest64[0] = 0;
    digest64[1] = 0;
    digest64[2] = 0;

    hmac_sha1(inData, inFileSize, key, 64, digest);

    printf("0x%016lx\n", endswap64(digest64[0]));
    printf("0x%016lx\n", endswap64(digest64[1]));
    printf("0x%016lx\n", endswap64(digest64[2]));

    free(inData);
}

int main(int argc, char **argv)
{
    if (argc == 5 && !strcmp(argv[1], "lv2diff"))
        lv2diff(argv[2], argv[3], argv[4]);
    else if (argc == 4 && !strcmp(argv[1], "lv2decrypt"))
        lv2decrypt(argv[2], argv[3]);
    else if (argc == 3 && !strcmp(argv[1], "lv2hash"))
        lv2hash(argv[2]);
    else
    {
        printf("lv2diff <inFile1> <inFile2> <outFile>\n");
        printf("lv2decrypt <inFile> <outFile>\n");
        printf("lv2hash <inFile>\n");
    }

    return 0;
}