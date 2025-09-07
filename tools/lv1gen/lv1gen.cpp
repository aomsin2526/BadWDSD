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

uint32_t endswap32(uint32_t v)
{
    return __bswap_32(v);
}

uint64_t endswap64(uint64_t v)
{
    return __bswap_64(v);
}

struct Zelf_Header_s
{
public:
    uint64_t magic;

    uint64_t original_size;
    uint64_t compressed_size;
};

bool SearchAndReplace(void *in_data, uint64_t dataSize, const void *in_searchData, uint64_t searchDataSize, const void *in_replaceData, uint64_t replaceDataSize)
{
    uint8_t *data = (uint8_t *)in_data;

    const uint8_t *searchData = (const uint8_t *)in_searchData;
    const uint8_t *replaceData = (const uint8_t *)in_replaceData;

    for (uint64_t i = 0; i < dataSize; ++i)
    {
        if (!memcmp(&data[i], searchData, searchDataSize))
        {
            memcpy(&data[i], replaceData, replaceDataSize);
            return true;
        }
    }

    return false;
}

void lv1gen(bool is4j, const char *inFilePath, const char *outFilePath, const char *stage3jFilePath, const char *stage3jaFilePath, const char *stage3jz4jFilePath, const char *stage5jFilePath, const char *stage6jFilePath)
{
    printf("lv1gen()\n");

    printf("is4j = %d\n", is4j ? 1 : 0);

    printf("inFilePath = %s\n", inFilePath);
    printf("outFilePath = %s\n", outFilePath);

    printf("stage3jFilePath = %s\n", stage3jFilePath);
    printf("stage3jaFilePath = %s\n", stage3jaFilePath);
    printf("stage3jz4jFilePath = %s\n", stage3jz4jFilePath);

    printf("stage5jFilePath = %s\n", stage5jFilePath);
    printf("stage6jFilePath = %s\n", stage6jFilePath);

    FILE *inFile = fopen(inFilePath, "rb");
    FILE *outFile = fopen(outFilePath, "wb");

    FILE *stage3jFile = fopen(stage3jFilePath, "rb");
    FILE *stage3jaFile = fopen(stage3jaFilePath, "rb");
    FILE *stage3jz4jFile = fopen(stage3jz4jFilePath, "rb");
    FILE *stage5jFile = fopen(stage5jFilePath, "rb");
    FILE *stage6jFile = fopen(stage6jFilePath, "rb");

    if (inFile == NULL || outFile == NULL || stage3jFile == NULL || stage3jaFile == NULL || stage3jz4jFile == NULL || stage5jFile == NULL|| stage6jFile == NULL)
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

    //

    size_t stage3jFileSize = get_file_size(stage3jFile);
    printf("stage3jFileSize = %lu\n", stage3jFileSize);

    if (stage3jFileSize > 16)
    {
        printf("bad stage3j file size!\n");

        abort();
        return;
    }

    uint8_t *stage3jData = (uint8_t *)malloc(stage3jFileSize);

    if (stage3jData == NULL)
    {
        printf("stage3jData failed!\n");

        abort();
        return;
    }

    fread(stage3jData, 1, stage3jFileSize, stage3jFile);
    fclose(stage3jFile);

    //

    size_t stage3jaFileSize = get_file_size(stage3jaFile);
    printf("stage3jaFileSize = %lu\n", stage3jaFileSize);

    if (stage3jaFileSize != 16)
    {
        printf("bad stage3ja file size!\n");

        abort();
        return;
    }

    uint8_t *stage3jaData = (uint8_t *)malloc(stage3jaFileSize);

    if (stage3jaData == NULL)
    {
        printf("stage3jaData failed!\n");

        abort();
        return;
    }

    fread(stage3jaData, 1, stage3jaFileSize, stage3jaFile);
    fclose(stage3jaFile);

    //

    size_t stage3jz4jFileSize = get_file_size(stage3jz4jFile);
    printf("stage3jz4jFileSize = %lu\n", stage3jz4jFileSize);

    if (is4j)
    {
        if (stage3jz4jFileSize > 644)
        {
            printf("bad stage4j file size!\n");

            abort();
            return;
        }
    }
    else
    {
        if (stage3jz4jFileSize != 28)
        {
            printf("bad stage3jz file size!\n");

            abort();
            return;
        }
    }

    uint8_t *stage3jz4jData = (uint8_t *)malloc(stage3jz4jFileSize);

    if (stage3jz4jData == NULL)
    {
        printf("stage3jz4jData failed!\n");

        abort();
        return;
    }

    fread(stage3jz4jData, 1, stage3jz4jFileSize, stage3jz4jFile);
    fclose(stage3jz4jFile);

    //

    size_t stage5jFileSize = get_file_size(stage5jFile);
    printf("stage5jFileSize = %lu\n", stage5jFileSize);

    if (stage5jFileSize != 20)
    {
        printf("bad stage5j file size!\n");

        abort();
        return;
    }

    uint8_t *stage5jData = (uint8_t *)malloc(stage5jFileSize);

    if (stage5jData == NULL)
    {
        printf("stage5jData failed!\n");

        abort();
        return;
    }

    fread(stage5jData, 1, stage5jFileSize, stage5jFile);
    fclose(stage5jFile);

    //

    size_t stage6jFileSize = get_file_size(stage6jFile);
    printf("stage6jFileSize = %lu\n", stage6jFileSize);

    if (stage6jFileSize != 20)
    {
        printf("bad stage6j file size!\n");

        abort();
        return;
    }

    uint8_t *stage6jData = (uint8_t *)malloc(stage6jFileSize);

    if (stage6jData == NULL)
    {
        printf("stage6jData failed!\n");

        abort();
        return;
    }

    fread(stage6jData, 1, stage6jFileSize, stage6jFile);
    fclose(stage6jFile);

    //

    uint8_t *outData = (uint8_t *)malloc(inFileSize);

    if (outData == NULL)
    {
        printf("malloc failed!\n");

        abort();
        return;
    }

    memcpy(outData, inData, inFileSize);

#if 1

    {
        printf("Writing 0x2401F031200 to offset 0x10120\n");

        *((uint64_t *)&outData[0x10120]) = endswap64(0x2401F031200);
    }

#endif

#if 1

    {
        uint8_t searchData[] = { 0x38, 0x00, 0xFF, 0xEC, 0xF8, 0x03, 0x00, 0x28, 0x38, 0x60, 0xFF, 0xEC, 0x4E, 0x80, 0x00, 0x20 };
        
        printf("Installing stage3j...\n");

        if (!SearchAndReplace(outData, inFileSize, searchData, 16, stage3jData, stage3jFileSize))
        {
            printf("install failed!\n");
        
            abort();
            return;
        }
    }

#endif

#if 1

    {
        uint8_t searchData[] = { 0x38, 0x80, 0x00, 0x02, 0xE8, 0xA2, 0x89, 0x18, 0x7F, 0x83, 0xE3, 0x78, 0x48, 0x00, 0xF5, 0xE5 };
        
        printf("Installing stage3ja...\n");

        if (!SearchAndReplace(outData, inFileSize, searchData, sizeof(searchData), stage3jaData, stage3jaFileSize))
        {
            printf("install failed!\n");
        
            abort();
            return;
        }
    }

#endif

    if (is4j)
    {
#if 1
        uint8_t searchData[] = { 0xF8, 0x21, 0xFF, 0x51, 0x7C, 0x08, 0x02, 0xA6, 0xFB, 0x61, 0x00, 0x88, 0xFB, 0x81, 0x00, 0x90, 0xFB, 0xC1, 0x00, 0xA0, 0x7C, 0x7C, 0x1B, 0x78 };
        
        printf("Installing stage4j...\n");

        if (!SearchAndReplace(outData, inFileSize, searchData, 24, stage3jz4jData, stage3jz4jFileSize))
        {
            printf("install failed!\n");

            abort();
            return;
        }
#endif
    }
    else
    {
#if 1
        uint8_t searchData[] = { 0xE8, 0xA2, 0x84, 0x48, 0x38, 0x80, 0x00, 0x02, 0xE8, 0x62, 0x83, 0x18, 0x7F, 0xE6, 0x07, 0xB4, 0x48, 0x00, 0x59, 0x2D };
        
        printf("Installing stage3jz...\n");

        if (!SearchAndReplace(outData, inFileSize, searchData, 20, stage3jz4jData, stage3jz4jFileSize))
        {
            printf("install failed!\n");
        
            abort();
            return;
        }
#endif
    }

#if 1

    {
        printf("Writing 0x2401F031400 to offset 0x10210\n");

        *((uint64_t *)&outData[0x10210]) = endswap64(0x2401F031400);
    }

#endif

#if 1

    {
        uint8_t searchData[] = { 0xE9, 0x22, 0xCF, 0x08, 0x7C, 0x80, 0x23, 0x78, 0x7C, 0xA6, 0x2B, 0x78 };
        
        printf("Installing stage5j (appldr)...\n");
        stage5jData[3] = 0x1;

        if (!SearchAndReplace(outData, inFileSize, searchData, sizeof(searchData), stage5jData, stage5jFileSize))
        {
            printf("install failed!\n");
        
            abort();
            return;
        }
    }

#endif

#if 1

    {
        uint8_t searchData[] = { 0xE9, 0x22, 0xCF, 0x08, 0x7C, 0x60, 0x1B, 0x78, 0x7C, 0x8B, 0x23, 0x78, 0x7C, 0xAA, 0x2B, 0x78, 0x7C, 0xC7, 0x33, 0x78 };
        
        printf("Installing stage5j (lv2ldr)...\n");
        stage5jData[3] = 0x2;

        if (!SearchAndReplace(outData, inFileSize, searchData, sizeof(searchData), stage5jData, stage5jFileSize))
        {
            printf("install failed!\n");
        
            abort();
            return;
        }
    }

#endif

#if 1

    {
        printf("Writing 0x2401F031700 to offset 0x10220\n");

        *((uint64_t *)&outData[0x10220]) = endswap64(0x2401F031700);
    }

#endif

#if 1

    {
        uint8_t searchData[] = { 0xE9, 0x22, 0xCF, 0x20, 0x78, 0x63, 0x9B, 0x24, 0x38, 0x00, 0x00, 0x03, 0xE9, 0x29, 0x00, 0x00, 0x3D, 0x29, 0x00, 0x04, 0x39, 0x29, 0x40, 0x1C, 0x7C, 0x03, 0x49, 0x2E };
        
        printf("Installing stage6j (SPU_iso_load_request)...\n");
        stage6jData[3] = 1;

        if (!SearchAndReplace(outData, inFileSize, searchData, sizeof(searchData), stage6jData, stage6jFileSize))
        {
            printf("install failed!\n");
        
            abort();
            return;
        }
    }

#endif

#if 1

    {
        uint8_t searchData[] = { 0x7C, 0x00, 0x06, 0xAC, 0xE9, 0x22, 0xCF, 0x20, 0x78, 0x63, 0x9B, 0x24, 0xE9, 0x29, 0x00, 0x00, 0x3D, 0x29, 0x00, 0x04, 0x39, 0x29, 0x40, 0x24, 0x7C, 0x63, 0x48, 0x2E };
        
        printf("Installing stage6j (__get_spu_status)...\n");
        stage6jData[3] = 2;

        if (!SearchAndReplace(outData, inFileSize, searchData, sizeof(searchData), stage6jData, stage6jFileSize))
        {
            printf("install failed!\n");
        
            abort();
            return;
        }
    }

#endif

#if 1

    {
        uint8_t searchData[] = { 0xE9, 0x22, 0xCF, 0x20, 0x78, 0x63, 0x9B, 0x24, 0x38, 0x00, 0x00, 0x02, 0xE9, 0x29, 0x00, 0x00, 0x3D, 0x29, 0x00, 0x04, 0x39, 0x29, 0x40, 0x1C, 0x7C, 0x03, 0x49, 0x2E };
        
        printf("Installing stage6j (request_exit_isolation)...\n");
        stage6jData[3] = 3;

        if (!SearchAndReplace(outData, inFileSize, searchData, sizeof(searchData), stage6jData, stage6jFileSize))
        {
            printf("install failed!\n");
        
            abort();
            return;
        }
    }

#endif

#if 1

    {
        uint8_t searchData[] = { 0xE9, 0x43, 0x00, 0x00, 0xE9, 0x63, 0x00, 0x08, 0xE9, 0x2A, 0x00, 0x08, 0x3D, 0x29, 0x00, 0x04, 0x80, 0x09, 0x40, 0x24, 0x90, 0x0B, 0x00, 0x30, 0xE9, 0x2B, 0x0F, 0x10, 0x7C, 0x84, 0x4B, 0x78, 0xF8, 0x8B, 0x0F, 0x10 };
        
        printf("Installing stage6j (SPE_update_SPU_status_and_transition_notifier_in_shadow_reg_area)...\n");
        stage6jData[3] = 4;

        if (!SearchAndReplace(outData, inFileSize, searchData, sizeof(searchData), stage6jData, stage6jFileSize))
        {
            printf("install failed!\n");
        
            abort();
            return;
        }
    }

#endif

#if 1

    {
        // Hvcall 114

        {
            uint8_t searchData[] = {0x2F, 0x80, 0x00, 0x00, 0x41, 0x9E, 0x00, 0x28, 0x38, 0x60, 0x00, 0x00, 0x38, 0x80, 0x00, 0x00};
            uint8_t replaceData[] = {0x60, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x28, 0x38, 0x60, 0x00, 0x00, 0x38, 0x80, 0x00, 0x00};

            printf("Patching hvcall 114 1...\n");

            if (!SearchAndReplace(outData, inFileSize, searchData, 16, replaceData, 16))
            {
                printf("patch failed!\n");

                //abort();
                //return;
            }
        }

        {
            uint8_t searchData[] = {0x00, 0x4B, 0xFF, 0xFB, 0xFD, 0x7C, 0x60, 0x1B};
            uint8_t replaceData[] = {0x01, 0x4B, 0xFF, 0xFB, 0xFD, 0x7C, 0x60, 0x1B};

            printf("Patching hvcall 114 2...\n");

            if (!SearchAndReplace(outData, inFileSize, searchData, 8, replaceData, 8))
            {
                printf("patch failed!\n");

                //abort();
                //return;
            }
        }
    }

#endif

    fwrite(outData, 1, inFileSize, outFile);

    free(outData);

    free(stage6jData);
    free(stage5jData);

    free(stage3jz4jData);
    free(stage3jaData);
    free(stage3jData);

    free(inData);

    fclose(outFile);
}

void lv1diff(const char *inFilePath1, const char *inFilePath2, const char *outFilePath)
{
    printf("lv1diff()\n");

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

            val |= (uint8_t)inData1[i]; // orig
            val <<= 8;
            val |= (uint8_t)inData2[i]; // new

            // shitty code

            if (addr >= 0x3f0000)
                addr += (0x600000 - 0x3f0000);
            else if (addr >= 0x1f0000)
                addr += (0x400000 - 0x1f0000);
            else if (addr >= 0x20000)
                addr += (0x200000 - 0x20000);
            else if (addr >= 0x10000)
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

int main(int argc, char **argv)
{
    if (argc == 9 && !strcmp(argv[1], "lv1gen_3jz"))
        lv1gen(false, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
    else if (argc == 9 && !strcmp(argv[1], "lv1gen_4j"))
        lv1gen(true, argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
    else if (argc == 5 && !strcmp(argv[1], "lv1diff"))
        lv1diff(argv[2], argv[3], argv[4]);
    else
    {
        printf("lv1gen_3jz <inFile> <outFile> <stage3j> <stage3ja> <stage3jz> <stage5j>\n");
        printf("lv1gen_4j <inFile> <outFile> <stage3j> <stage3ja> <stage4j> <stage5j>\n");

        printf("lv1diff <inFile1> <inFile2> <outFile>\n");
    }

    return 0;
}