#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/stat.h>

#include <dirent.h>

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

void extract_coreos_from_nor_dump(const char *dumpPath, const char *outDir)
{
    printf("extract_coreos_from_nor_dump()\n");

    printf("dumpPath = %s\n", dumpPath);
    printf("outDir = %s\n", outDir);

    FILE *dumpFile = fopen(dumpPath, "rb");

    if (dumpFile == NULL)
    {
        printf("open dumpFile fail!\n");

        abort();
        return;
    }

    size_t dumpFileSize = get_file_size(dumpFile);
    printf("dumpFile size = %lu\n", dumpFileSize);

    if (dumpFileSize != (16 * 1024 * 1024))
    {
        printf("bad size!\n");

        abort();
        return;
    }

    uint8_t *dumpData = (uint8_t *)malloc(dumpFileSize);

    if (dumpData == NULL)
    {
        printf("alloc dumpData fail!\n");

        abort();
        return;
    }

    fread(dumpData, 1, dumpFileSize, dumpFile);
    fclose(dumpFile);

    {
        char outPath[1024];
        sprintf(outPath, "%s/ros0", outDir);

        FILE *outFile = fopen(outPath, "wb");

        if (outFile == NULL)
        {
            printf("open outFile fail!\n");

            abort();
            return;
        }

        fwrite(dumpData + 0x0C0000, 1, 0x700000, outFile);
        fclose(outFile);
    }

    {
        char outPath[1024];
        sprintf(outPath, "%s/ros1", outDir);

        FILE *outFile = fopen(outPath, "wb");

        if (outFile == NULL)
        {
            printf("open outFile fail!\n");

            abort();
            return;
        }

        fwrite(dumpData + 0x7C0000, 1, 0x700000, outFile);
        fclose(outFile);
    }

    free(dumpData);

    printf("extract_coreos_from_nor_dump() done\n");
}

struct coreos_header_s
{
public:
    uint64_t unknown0;
    uint64_t length_region;
    uint32_t unknown1;
    uint32_t entry_count;
    uint64_t length_region2;
};

struct coreos_entry_s
{
public:
    uint64_t offset;
    uint64_t length;
    char file_name[32];
};

void extract_coreos(const char *rosPath, const char *outDir)
{
    printf("extract_coreos()\n");

    printf("rosPath = %s\n", rosPath);
    printf("outDir = %s\n", outDir);

    FILE *rosFile = fopen(rosPath, "rb");

    if (rosFile == NULL)
    {
        printf("open rosFile fail!\n");

        abort();
        return;
    }

    size_t rosFileSize = get_file_size(rosFile);

    if (rosFileSize > 0x700000)
    {
        printf("bad file size!\n");

        abort();
        return;
    }

    uint8_t *rosData = (uint8_t *)malloc(rosFileSize + 16);

    if (rosData == NULL)
    {
        printf("alloc rosData fail!\n");

        abort();
        return;
    }

    fread(rosData, 1, rosFileSize, rosFile);
    fclose(rosFile);

    if (*((uint64_t*)rosData) != 0)
        memmove(rosData + 16, rosData, rosFileSize);

    {
        size_t curOffset = 0;

        coreos_header_s header;
        memcpy(&header, rosData + curOffset, sizeof(coreos_header_s));
        curOffset += sizeof(coreos_header_s);

        header.unknown0 = endswap64(header.unknown0);
        header.length_region = endswap64(header.length_region);
        header.unknown1 = endswap32(header.unknown1);
        header.entry_count = endswap32(header.entry_count);
        header.length_region2 = endswap64(header.length_region2);

        printf("entryCount = %u\n", header.entry_count);

        coreos_entry_s *entrys = new coreos_entry_s[header.entry_count];

        for (uint64_t i = 0; i < header.entry_count; ++i)
        {
            coreos_entry_s *entry = &entrys[i];

            memcpy(entry, rosData + curOffset, sizeof(coreos_entry_s));
            curOffset += sizeof(coreos_entry_s);

            entry->offset = endswap64(entry->offset);
            entry->length = endswap64(entry->length);
        }

        for (uint64_t i = 0; i < header.entry_count; ++i)
        {
            const coreos_entry_s *entry = &entrys[i];

            printf("file_name = %s, offset = 0x%lx, length = %lu\n", entry->file_name, entry->offset, entry->length);

            char outPath[1024];
            sprintf(outPath, "%s/%s", outDir, entry->file_name);

            FILE *outFile = fopen(outPath, "wb");

            if (outFile == NULL)
            {
                printf("open outFile fail!\n");

                abort();
                return;
            }

            fwrite(rosData + (entry->offset + 16), 1, entry->length, outFile);
            fclose(outFile);
        }

        delete[] entrys;
    }

    printf("extract_coreos() done\n");
}

void create_coreos(const char *inDir, const char *rosPath)
{
    printf("create_coreos()\n");

    printf("inDir = %s\n", inDir);
    printf("rosPath = %s\n", rosPath);

    DIR *dp;
    dp = opendir(inDir);

    if (dp == NULL)
    {
        printf("opendir fail!\n");

        abort();
        return;
    }

    FILE *rosFile = fopen(rosPath, "wb");

    if (rosFile == NULL)
    {
        printf("open rosFile fail!\n");

        abort();
        return;
    }

    {
        dirent *dent;

        coreos_header_s header;
        header.unknown0 = 0;
        header.length_region = 0x6FFFE0;
        header.unknown1 = 1;
        header.entry_count = 0;
        header.length_region2 = 0x6FFFE0;

        coreos_entry_s *entrys = new coreos_entry_s[256];

        while (1)
        {
            dent = readdir(dp);

            if (dent == NULL)
                break;

            if (dent->d_type != DT_REG)
                continue;

            if (header.entry_count == 256)
            {
                printf("too many entry!\n");

                abort();
                return;
            }

            char path[1024];
            sprintf(path, "%s/%s", inDir, dent->d_name);

            FILE *f = fopen(path, "rb");

            if (f == NULL)
            {
                printf("open file fail!\n");

                abort();
                return;
            }

            coreos_entry_s *entry = &entrys[header.entry_count];
            ++header.entry_count;

            if (strlen(dent->d_name) > 31)
            {
                printf("file name too long\n");

                abort();
                return;
            }

            strcpy(entry->file_name, dent->d_name);
            entry->length = get_file_size(f);

            fclose(f);
        }

        printf("entry_count = %u\n", header.entry_count);

        uint64_t curOffset = 0;
        curOffset += sizeof(coreos_header_s);
        curOffset += header.entry_count * sizeof(coreos_entry_s);

        for (uint32_t i = 0; i < header.entry_count; ++i)
        {
            coreos_entry_s *entry = &entrys[i];

            curOffset = round_up(curOffset, 0x20);

            entry->offset = (curOffset - 16);
            curOffset += entry->length;
        }

        if (curOffset > 0x700000)
        {
            printf("result ros too big!\n");

            abort();
            return;
        }

        header.unknown0 = endswap64(header.unknown0);
        header.length_region = endswap64(header.length_region);
        header.unknown1 = endswap32(header.unknown1);
        header.entry_count = endswap32(header.entry_count);
        header.length_region2 = endswap64(header.length_region2);

        fwrite(&header, 1, sizeof(coreos_header_s), rosFile);

        header.unknown0 = endswap64(header.unknown0);
        header.length_region = endswap64(header.length_region);
        header.unknown1 = endswap32(header.unknown1);
        header.entry_count = endswap32(header.entry_count);
        header.length_region2 = endswap64(header.length_region2);

        for (uint32_t i = 0; i < header.entry_count; ++i)
        {
            coreos_entry_s *entry = &entrys[i];

            entry->offset = endswap64(entry->offset);
            entry->length = endswap64(entry->length);

            fwrite(entry, 1, sizeof(coreos_entry_s), rosFile);

            entry->offset = endswap64(entry->offset);
            entry->length = endswap64(entry->length);
        }

        for (uint32_t i = 0; i < header.entry_count; ++i)
        {
            coreos_entry_s *entry = &entrys[i];

            printf("file_name = %s, offset = 0x%lx, length = %lu\n", entry->file_name, entry->offset, entry->length);

            char path[1024];
            sprintf(path, "%s/%s", inDir, entry->file_name);

            FILE *f = fopen(path, "rb");

            if (f == NULL)
            {
                printf("open file fail!\n");

                abort();
                return;
            }

            uint8_t *buff = (uint8_t *)malloc(entry->length);

            if (buff == NULL)
            {
                printf("alloc buff fail!\n");

                abort();
                return;
            }

            if (get_file_size(f) != entry->length)
            {
                printf("file size mismatch!\n");

                abort();
                return;
            }

            fseek(rosFile, entry->offset + 16, SEEK_SET);

            fread(buff, 1, entry->length, f);
            fclose(f);

            fwrite(buff, 1, entry->length, rosFile);

            free(buff);
        }

        if (get_file_size(rosFile) != curOffset)
        {
            printf("resulting ros file not match expected value!\n");

            abort();
            return;
        }

        delete[] entrys;
    }

    fclose(rosFile);

    closedir(dp);

    printf("create_coreos() done\n");
}

int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[1], "extract_coreos_from_nor_dump"))
        extract_coreos_from_nor_dump(argv[2], argv[3]);
    else if (argc == 4 && !strcmp(argv[1], "extract_coreos"))
        extract_coreos(argv[2], argv[3]);
    else if (argc == 4 && !strcmp(argv[1], "create_coreos"))
        create_coreos(argv[2], argv[3]);
    else
    {
        printf("create_coreos <inDir> <rosFile>\n");
        printf("extract_coreos <dumpFile> <outDir>\n");
        printf("extract_coreos_from_nor_dump <dumpFile> <outDir>\n");
    }

    return 0;
}