#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/stat.h>

#include "miniz.c"

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

struct Zelf2_Header_s
{
public:
    uint64_t magic;

    uint64_t original_size;
    uint64_t compressed_size;

    uint64_t padding;
};

void Zelf_Gen(const char* inFilePath, const char* outFilePath)
{
    printf("Zelf_Gen()\n");
    
    printf("inFilePath = %s\n", inFilePath);
    printf("outFilePath = %s\n", outFilePath);

    FILE* inFile = fopen(inFilePath, "rb");

    if (inFile == NULL)
    {
        printf("inFile open failed!\n");

        abort();
        return;
    }

    size_t inFileSize = get_file_size(inFile);
    printf("inFileSize = %lu\n", inFileSize);

    uint8_t* inData = (uint8_t*)malloc(inFileSize);

    if (inData == NULL)
    {
        printf("inData alloc failed!\n");

        abort();
        return;
    }

    fread(inData, 1, inFileSize, inFile);
    fclose(inFile);

    FILE* outFile = fopen(outFilePath, "wb");

    if (outFile == NULL)
    {
        printf("outFile open failed!\n");

        abort();
        return;
    }

    uint64_t compressedDataSize = (inFileSize * 2);
    uint8_t* compressedData = (uint8_t*)malloc(compressedDataSize);

    if (compressedData == NULL)
    {
        printf("compressedData alloc failed!\n");

        abort();
        return;
    }

    int32_t cmp_static = mz_compress2(compressedData, &compressedDataSize, inData, inFileSize, MZ_BEST_COMPRESSION);

    if (cmp_static != Z_OK)
    {
        printf("compress failed!, res = %d\n", cmp_static);

        abort();
        return;
    }

    printf("compressedDataSize = %lu\n", compressedDataSize);

    {
        Zelf2_Header_s hdr;

        hdr.magic = endswap64(0x5A454C465A454C32); // ZELF2

        hdr.original_size = endswap64(inFileSize);
        hdr.compressed_size = endswap64(compressedDataSize);

        hdr.padding = 0;

        fwrite(&hdr, 1, sizeof(Zelf2_Header_s), outFile);
    }

    fwrite(compressedData, 1, compressedDataSize, outFile);

    free(compressedData);
    free(inData);

    fclose(outFile);
}

int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[1], "zelf_gen"))
        Zelf_Gen(argv[2], argv[3]);
    else
    {
        printf("zelf_gen <inFile> <outFile>\n");
    }

    return 0;
}