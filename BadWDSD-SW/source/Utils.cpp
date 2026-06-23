#include "Include.h"

#define SYS_TIMEBASE_GET(tb)                          \
	do                                                \
	{                                                 \
		__asm__ volatile("1: mftb %[current_tb];"     \
						 "cmpwi 7, %[current_tb], 0;" \
						 "beq-  7, 1b;"               \
						 : [current_tb] "=r"(tb) :    \
						 : "cr7");                    \
	} while (0)

uint64_t my_timebase = sysGetTimebaseFrequency();

uint64_t GetTimeInNs()
{
	uint64_t cur_tb_ns;
	SYS_TIMEBASE_GET(cur_tb_ns);

	cur_tb_ns *= MUL_NS;

	return (cur_tb_ns / my_timebase);
}

void WaitInNs(uint64_t ns)
{
	uint64_t start = GetTimeInNs();
	uint64_t end = start + ns;

	while (1)
	{
		uint64_t cur = GetTimeInNs();

		if (cur >= end)
			return;
	}
}

uint64_t GetTimeInUs()
{
	uint64_t cur_tb_us;
	SYS_TIMEBASE_GET(cur_tb_us);

	cur_tb_us *= MUL_US;

	return (cur_tb_us / my_timebase);
}

void WaitInUs(uint64_t us)
{
	uint64_t start = GetTimeInUs();
	uint64_t end = start + us;

	while (1)
	{
		uint64_t cur = GetTimeInUs();

		if (cur >= end)
			return;
	}
}

uint64_t GetTimeInMs()
{
	uint64_t cur_tb_ms;
	SYS_TIMEBASE_GET(cur_tb_ms);

	cur_tb_ms *= MUL_MS;

	return (cur_tb_ms / my_timebase);
}

void WaitInMs(uint64_t ms)
{
	uint64_t start = GetTimeInMs();
	uint64_t end = start + ms;

	while (1)
	{
		uint64_t cur = GetTimeInMs();

		if (cur >= end)
			return;
	}
}

void Sleep(uint32_t secs)
{
	sysSleep(secs);
}

bool IsFileExist(const char* path)
{
	FILE* f = fopen(path, "rb");

	if (f == NULL)
		return false;

	fclose(f);
	return true;
}

size_t GetFileSize(FILE* f)
{
	size_t old = ftell(f);

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);

	fseek(f, old, SEEK_SET);
	return size;
}

bool FlashIsNor()
{
	uint8_t flag;

	int32_t res = lv2_storage_get_cache_of_flash_ext_flag(&flag);

	if (res != 0)
	{
		PrintLog("lv2_storage_get_cache_of_flash_ext_flag failed!, res = %d\n", res);

		abort();
		return false;
	}

	return !(flag & 0x1);
}

bool TargetIsCEX()
{
	uint64_t type;

	int32_t res = lv2_dbg_get_console_type(&type);

	if (res != 0)
	{
		PrintLog("lv2_dbg_get_console_type failed!, res = %d\n", res);

		abort();
		return false;
	}

	return (type == 1);
}

bool TargetIsDEX()
{
	uint64_t type;

	int32_t res = lv2_dbg_get_console_type(&type);

	if (res != 0)
	{
		PrintLog("lv2_dbg_get_console_type failed!, res = %d\n", res);

		abort();
		return false;
	}

	return (type == 2);
}

bool TargetIsDECR()
{
	uint64_t type;

	int32_t res = lv2_dbg_get_console_type(&type);

	if (res != 0)
	{
		PrintLog("lv2_dbg_get_console_type failed!, res = %d\n", res);

		abort();
		return false;
	}

	return (type == 3);
}

void NorWrite(uint64_t offset, const void* data, uint64_t size)
{
	const uint8_t* dataa = (const uint8_t*)data;

	PrintLog("NorWrite() offset = 0x%lx, data = 0x%lx, size = %lu\n", offset, (uint64_t)data, size);

	if (data == NULL)
		return;

	if (size == 0)
		return;

	if (!FlashIsNor())
	{
		PrintLog("Flash is not nor!\n");

		abort();
		return;
	}

	int32_t res;

	uint32_t unknown2;

	uint64_t dev_id = 0x100000000000004ull;
	uint64_t dev_flags = 0x22ull;

	static const uint64_t sector_size = 512;
	uint64_t burst_size = (512 * sector_size);

	// example workaround to fix 0xf31000 bug
	if (burst_size > (64 * 1024))
		burst_size = (64 * 1024);

	PrintLog("burst_size = %lu\n", burst_size);

	uint32_t dev_handle;

	res = lv2_storage_open(dev_id, &dev_handle);

	if (res != 0)
	{
		PrintLog("lv2_storage_open failed!, res = %d\n", res);

		abort();
		return;
	}

	char* buf = (char*)malloc(burst_size);

	if (buf == NULL)
	{
		PrintLog("malloc failed!\n");

		abort();
		return;
	}

	uint64_t curOffset = offset;
	uint64_t curDataOffset = 0;

	uint64_t left = size;

	while (left > 0)
	{
		uint64_t processSize = (left > sector_size) ? sector_size : left;
		uint64_t zzz = (curOffset % sector_size);
		uint64_t yyy = (sector_size - zzz);
		uint64_t xxx = (yyy > processSize) ? processSize : yyy;

		uint64_t sector_idx = (curOffset / sector_size);

		PrintLog("curOffset = 0x%lx, curDataOffset = 0x%lx, processSize = %lu, zzz = %lu, yyy = %lu, xxx = %lu, sector_idx = %lu, left = %lu\n",
			curOffset, curDataOffset, processSize, zzz, yyy, xxx, sector_idx, left);

		if (burst_size > left)
		{
			while (burst_size > left)
				burst_size -= sector_size;

			PrintLog("burst_size = %lu\n", burst_size);
		}

		if ((zzz != 0) || (processSize != sector_size))
		{
			PrintLog("1\n");

			res = lv2_storage_read(dev_handle, 0, sector_idx, 1, buf, &unknown2, dev_flags);

			if (res != 0)
			{
				PrintLog("lv2_storage_read failed! res = %d\n", res);

				abort();
				return;
			}

			memcpy(&buf[zzz], &dataa[curDataOffset], xxx);

			res = lv2_storage_write(dev_handle, 0, sector_idx, 1, buf, &unknown2, dev_flags);

			if (res != 0)
			{
				PrintLog("lv2_storage_write failed! res = %d\n", res);

				abort();
				return;
			}

			curOffset += xxx;
			curDataOffset += xxx;

			left -= xxx;
		}
		else if ((burst_size > 0) && (left >= burst_size) && ((burst_size % sector_size) == 0))
		{
			PrintLog("2\n");

			memcpy(&buf[0], &dataa[curDataOffset], burst_size);

			res = lv2_storage_write(dev_handle, 0, sector_idx, (burst_size / sector_size), buf, &unknown2, dev_flags);

			if (res != 0)
			{
				PrintLog("lv2_storage_write failed! res = %d\n", res);

				abort();
				return;
			}

			curOffset += burst_size;
			curDataOffset += burst_size;

			left -= burst_size;
		}
		else
		{
			PrintLog("3\n");

			memcpy(&buf[0], &dataa[curDataOffset], processSize);

			res = lv2_storage_write(dev_handle, 0, sector_idx, 1, buf, &unknown2, dev_flags);

			if (res != 0)
			{
				PrintLog("lv2_storage_write failed! res = %d\n", res);

				abort();
				return;
			}

			curOffset += processSize;
			curDataOffset += processSize;

			left -= processSize;
		}
	}

	res = lv2_storage_close(dev_handle);

	if (res != 0)
	{
		PrintLog("lv2_storage_close failed!, res = %d\n", res);

		abort();
		return;
	}

	free(buf);

	PrintLog("NorWrite() done.\n");
}

void NorRead(uint64_t offset, void* data, uint64_t size)
{
	uint8_t* dataa = (uint8_t*)data;

	PrintLog("NorRead() offset = 0x%lx, data = 0x%lx, size = %lu\n", offset, (uint64_t)data, size);

	if (data == NULL)
		return;

	if (size == 0)
		return;

	if (!FlashIsNor())
	{
		PrintLog("Flash is not nor!\n");

		abort();
		return;
	}

	int32_t res;

	uint32_t unknown2;

	uint64_t dev_id = 0x100000000000004ull;
	uint64_t dev_flags = 0x22ull;

	static const uint64_t sector_size = 512;
	uint64_t burst_size = (512 * sector_size);

	PrintLog("burst_size = %lu\n", burst_size);

	uint32_t dev_handle;

	res = lv2_storage_open(dev_id, &dev_handle);

	if (res != 0)
	{
		PrintLog("lv2_storage_open failed!, res = %d\n", res);

		abort();
		return;
	}

	char* buf = (char*)malloc(burst_size);

	if (buf == NULL)
	{
		PrintLog("malloc failed!\n");

		abort();
		return;
	}

	uint64_t curOffset = offset;
	uint64_t curDataOffset = 0;

	uint64_t left = size;

	while (left > 0)
	{
		uint64_t processSize = (left > sector_size) ? sector_size : left;
		uint64_t zzz = (curOffset % sector_size);
		uint64_t yyy = (sector_size - zzz);
		uint64_t xxx = (yyy > processSize) ? processSize : yyy;

		uint64_t sector_idx = (curOffset / sector_size);

		PrintLog("curOffset = 0x%lx, curDataOffset = 0x%lx, processSize = %lu, zzz = %lu, yyy = %lu, xxx = %lu, sector_idx = %lu, left = %lu\n",
			curOffset, curDataOffset, processSize, zzz, yyy, xxx, sector_idx, left);

		if (burst_size > left)
		{
			while (burst_size > left)
				burst_size -= sector_size;

			PrintLog("burst_size = %lu\n", burst_size);
		}

		if ((zzz != 0) || (processSize != sector_size))
		{
			PrintLog("1\n");

			res = lv2_storage_read(dev_handle, 0, sector_idx, 1, buf, &unknown2, dev_flags);

			if (res != 0)
			{
				PrintLog("lv2_storage_read failed! res = %d\n", res);

				abort();
				return;
			}

			memcpy(&dataa[curDataOffset], &buf[zzz], xxx);

			curOffset += xxx;
			curDataOffset += xxx;

			left -= xxx;
		}
		else if ((burst_size > 0) && (left >= burst_size) && ((burst_size % sector_size) == 0))
		{
			PrintLog("2\n");

			res = lv2_storage_read(dev_handle, 0, sector_idx, (burst_size / sector_size), buf, &unknown2, dev_flags);

			if (res != 0)
			{
				PrintLog("lv2_storage_read failed! res = %d\n", res);

				abort();
				return;
			}

			memcpy(&dataa[curDataOffset], &buf[0], burst_size);

			curOffset += burst_size;
			curDataOffset += burst_size;

			left -= burst_size;
		}
		else
		{
			PrintLog("3\n");

			res = lv2_storage_read(dev_handle, 0, sector_idx, 1, buf, &unknown2, dev_flags);

			if (res != 0)
			{
				PrintLog("lv2_storage_read failed! res = %d\n", res);

				abort();
				return;
			}

			memcpy(&dataa[curDataOffset], &buf[0], processSize);

			curOffset += processSize;
			curDataOffset += processSize;

			left -= processSize;
		}
	}

	res = lv2_storage_close(dev_handle);

	if (res != 0)
	{
		PrintLog("lv2_storage_close failed!, res = %d\n", res);

		abort();
		return;
	}

	free(buf);

	PrintLog("NorRead() done.\n");
}