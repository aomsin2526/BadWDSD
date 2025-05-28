#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/stat.h>

size_t get_file_size(FILE *f)
{
	size_t old_off = ftell(f);

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);

	fseek(f, old_off, SEEK_SET);
	return size;
}

size_t get_cur_offset(FILE *f)
{
	return ftell(f);
}

uint32_t endswap16(uint16_t v)
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

void bin_to_elf(const char* inFilePath, const char* outFilePath)
{
	FILE *inFile = fopen(inFilePath, "rb");

	if (!inFile)
	{
		printf("input file not found!\n");

		abort();
		return;
	}

	size_t inFile_Size = get_file_size(inFile);

	//
	// inFile_Size -= 0x10000;
	// fseek(inFile, 0x10000, SEEK_SET);
	//

	void *inFileBuf = malloc(inFile_Size);
	fread(inFileBuf, 1, inFile_Size, inFile);

	fclose(inFile);

	FILE *outFile = fopen(outFilePath, "wb");

	if (!outFile)
	{
		printf("cant open output file!\n");
		
		abort();
		return;
	}

	// magic

	{
		uint32_t magic = endswap32(0x7f454c46);
		fwrite(&magic, 4, 1, outFile);
	}

	// 64 bit

	{
		uint8_t v = 0x02;
		fwrite(&v, 1, 1, outFile);
	}

	// big endian

	{
		uint8_t v = 0x02;
		fwrite(&v, 1, 1, outFile);
	}

	// version

	{
		uint8_t v = 0x01;
		fwrite(&v, 1, 1, outFile);
	}

	// os

	{
		uint8_t v = 0x66;
		fwrite(&v, 1, 1, outFile);
	}

	// abi version

	{
		uint8_t v = 0x0;
		fwrite(&v, 1, 1, outFile);
	}

	// padding

	{
		uint64_t v = 0x0;
		fwrite(&v, 7, 1, outFile);
	}

	// ET_EXEC

	{
		uint16_t v = endswap16(0x02);
		fwrite(&v, 2, 1, outFile);
	}

	// PPC64

	{
		uint16_t v = endswap16(0x15);
		fwrite(&v, 2, 1, outFile);
	}

	// version

	{
		uint32_t v = endswap32(0x1);
		fwrite(&v, 4, 1, outFile);
	}

	// entry

	{
		uint64_t v = endswap64(0x100);
		fwrite(&v, 8, 1, outFile);
	}

	// ph start

	{
		uint64_t v = endswap64(64);
		fwrite(&v, 8, 1, outFile);
	}

	// sh start

	{
		uint64_t v = endswap64(0);
		fwrite(&v, 8, 1, outFile);
	}

	// flags

	{
		uint32_t v = endswap32(0x0);
		fwrite(&v, 4, 1, outFile);
	}

	// size of this header

	{
		uint16_t v = endswap16(64);
		fwrite(&v, 2, 1, outFile);
	}

	// size of ph

	{
		uint16_t v = endswap16(56);
		fwrite(&v, 2, 1, outFile);
	}

	// ph count

	{
		uint16_t v = endswap16(1);
		fwrite(&v, 2, 1, outFile);
	}

	// size of sh

	{
		uint16_t v = endswap16(64);
		fwrite(&v, 2, 1, outFile);
	}

	// sh count

	{
		uint16_t v = endswap16(0);
		fwrite(&v, 2, 1, outFile);
	}

	// sh string

	{
		uint16_t v = endswap16(0);
		fwrite(&v, 2, 1, outFile);
	}

	// ph 1

	{
		// PT_LOAD

		{
			uint32_t v = endswap32(0x00000001);
			fwrite(&v, 4, 1, outFile);
		}

		// PF_X | PF_W | PF_R

		{
			uint32_t v = endswap32(0x1 | 0x2 | 0x4);
			fwrite(&v, 4, 1, outFile);
		}

		// p_offset

		{
			uint64_t v = endswap64(0x10000);
			fwrite(&v, 8, 1, outFile);
		}

		// p_vaddr

		{
			uint64_t v = endswap64(0x0);
			fwrite(&v, 8, 1, outFile);
		}

		// p_paddr

		{
			uint64_t v = endswap64(0x0);
			fwrite(&v, 8, 1, outFile);
		}

		// p_filesz

		{
			uint64_t v = endswap64(inFile_Size);
			fwrite(&v, 8, 1, outFile);
		}

		// p_memsz

		{
			uint64_t v = endswap64(inFile_Size);
			fwrite(&v, 8, 1, outFile);
		}

		// p_align

		{
			uint64_t v = endswap64(0x10000);
			fwrite(&v, 8, 1, outFile);
		}
	}

	// pad until size = 0x10000

	while (1)
	{
		uint8_t v = 0;
		fwrite(&v, 1, 1, outFile);

		size_t pos = get_cur_offset(outFile);

		if (pos == 0x10000)
			break;
	}

	// copy program data

	{
		fwrite(inFileBuf, 1, inFile_Size, outFile);
	}

	fclose(outFile);

	free(inFileBuf);
}

void replace_initramfs(const char* inFilePath, const char* outFilePath, const char* initramfsPath)
{
	// inFile size must be 9772164

	// initramfs max size = 4307335
	// offset = 0x428000

	printf("replace_initramfs()\n");

	printf("inFilePath = %s\n", inFilePath);
	printf("outFilePath = %s\n", outFilePath);

	printf("initramfsPath = %s\n", initramfsPath);

	//

	FILE* inFile = fopen(inFilePath, "rb");

	if (inFile == NULL)
	{
		printf("inFile open fail!\n");

		abort();
		return;
	}

	size_t inFileSize = get_file_size(inFile);
	printf("inFileSize = %lu\n", inFileSize);

	if (inFileSize != 9772164)
	{
		printf("bad inFile size!\n");

		abort();
		return;
	}

	uint8_t* inData = (uint8_t*)malloc(inFileSize);

	if (inData == NULL)
	{
		printf("inData alloc fail!\n");

		abort();
		return;
	}

	fread(inData, 1, inFileSize, inFile);
	fclose(inFile);

	//

	FILE* initramfsFile = fopen(initramfsPath, "rb");

	if (initramfsFile == NULL)
	{
		printf("initramfsFile open fail!\n");

		abort();
		return;
	}

	size_t initramfsFileSize = get_file_size(initramfsFile);
	printf("initramfsFileSize = %lu\n", initramfsFileSize);

	if (initramfsFileSize > 4307335)
	{
		printf("bad initramfs size!\n");

		abort();
		return;
	}

	uint8_t* initramfsData = (uint8_t*)malloc(initramfsFileSize);

	if (initramfsData == NULL)
	{
		printf("initramfsData alloc fail!\n");

		abort();
		return;
	}

	fread(initramfsData, 1, initramfsFileSize, initramfsFile);
	fclose(initramfsFile);

	//

	uint8_t* outData = (uint8_t*)malloc(inFileSize);

	if (outData == NULL)
	{
		printf("outData alloc fail!\n");

		abort();
		return;
	}

	memcpy(outData, inData, inFileSize);
	memset(outData + 0x428000, 0, 4307335);
	memcpy(outData + 0x428000, initramfsData, initramfsFileSize);

	//

	FILE* outFile = fopen(outFilePath, "wb");

	if (outFile == NULL)
	{
		printf("outFile open fail!\n");

		abort();
		return;
	}

	fwrite(outData, 1, inFileSize, outFile);
	fclose(outFile);

	//

	free(outData);

	free(initramfsData);
	free(inData);
}

int main(int argc, char **argv)
{
	if (argc == 3)
		bin_to_elf(argv[1], argv[2]);
	else if (argc == 5 && !strcmp(argv[1], "replace_initramfs"))
		replace_initramfs(argv[2], argv[3], argv[4]);
	else
	{
		printf("args: <dtbImage.ps3.bin> <dtbImage.ps3.elf>\n");
		printf("args: replace_initramfs <inFile> <outFile> <initramfs>\n");
	}
	
	return 0;
}
