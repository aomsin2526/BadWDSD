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

void lv1gen(const char *inFilePath, const char *outFilePath, const char *stage3jFilePath, const char *stage3jzFilePath, const char *stage5jFilePath)
{
    printf("lv1gen()\n");

    printf("inFilePath = %s\n", inFilePath);
    printf("outFilePath = %s\n", outFilePath);

    printf("stage3jFilePath = %s\n", stage3jFilePath);
    printf("stage3jzFilePath = %s\n", stage3jzFilePath);
    printf("stage5jFilePath = %s\n", stage5jFilePath);

    FILE *inFile = fopen(inFilePath, "rb");
    FILE *outFile = fopen(outFilePath, "wb");

    FILE *stage3jFile = fopen(stage3jFilePath, "rb");
    FILE *stage3jzFile = fopen(stage3jzFilePath, "rb");
    FILE *stage5jFile = fopen(stage5jFilePath, "rb");

    if (inFile == NULL || outFile == NULL || stage3jFile == NULL || stage3jzFile == NULL || stage5jFile == NULL)
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

    size_t stage3jzFileSize = get_file_size(stage3jzFile);
    printf("stage3jzFileSize = %lu\n", stage3jzFileSize);

    if (stage3jzFileSize != 28)
    {
        printf("bad stage3jz file size!\n");

        abort();
        return;
    }

    uint8_t *stage3jzData = (uint8_t *)malloc(stage3jzFileSize);

    if (stage3jzData == NULL)
    {
        printf("stage3jzData failed!\n");

        abort();
        return;
    }

    fread(stage3jzData, 1, stage3jzFileSize, stage3jzFile);
    fclose(stage3jzFile);

    //

    size_t stage5jFileSize = get_file_size(stage5jFile);
    printf("stage5jFileSize = %lu\n", stage5jFileSize);

    if (stage5jFileSize != 20)
    {
        printf("bad stage2j file size!\n");

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
        uint8_t searchData[] = { 0xE8, 0xA2, 0x84, 0x48, 0x38, 0x80, 0x00, 0x02, 0xE8, 0x62, 0x83, 0x18, 0x7F, 0xE6, 0x07, 0xB4, 0x48, 0x00, 0x59, 0x2D };
        
        printf("Installing stage3jz...\n");

        if (!SearchAndReplace(outData, inFileSize, searchData, 20, stage3jzData, stage3jzFileSize))
        {
            printf("install failed!\n");
        
            abort();
            return;
        }
    }

#endif

#if 1

    {
        printf("Writing 0x2401F031300 to offset 0x10210\n");

        *((uint64_t *)&outData[0x10210]) = endswap64(0x2401F031300);
    }

#endif

#if 1

    {
        uint8_t searchData[] = { 0x4B, 0xFF, 0xFC, 0x8D, 0x7C, 0x7E, 0x1B, 0x78, 0x38, 0x00, 0x00, 0x00, 0x7C, 0x01, 0x01, 0x64 };
        
        uint64_t replaceDataSize = sizeof(searchData) + stage5jFileSize;
        uint8_t replaceData[replaceDataSize];

        memcpy(replaceData, searchData, sizeof(searchData));
        memcpy(replaceData + sizeof(searchData), stage5jData, stage5jFileSize);

        printf("Installing stage5j...\n");

        if (!SearchAndReplace(outData, inFileSize, searchData, sizeof(searchData), replaceData, replaceDataSize))
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

    free(stage5jData);

    free(stage3jzData);
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
            uint32_t val = (uint8_t)inData2[i];

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
    if (argc == 7 && !strcmp(argv[1], "lv1gen"))
        lv1gen(argv[2], argv[3], argv[4], argv[5], argv[6]);
    else if (argc == 5 && !strcmp(argv[1], "lv1diff"))
        lv1diff(argv[2], argv[3], argv[4]);
    else
    {
        printf("lv1gen <inFile> <outFile> <stage3j> <stage3jz> <stage5j>\n");
        printf("lv1diff <inFile1> <inFile2> <outFile>\n");
    }

    return 0;
}