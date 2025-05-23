#include "Include.h"

void LoadLv2Kernel(const char *fileName, uint32_t type)
{
	PrintLog("LoadLv2Kernel(), fileName = %s\n", fileName);

	if (strlen(fileName) > 24)
	{
		PrintLog("file name too long! Abort!()\n");

		abort();
		return;
	}

	char fullPath[512];
	sprintf(fullPath, "/dev_flash/sys/%s", fileName);

	PrintLog("fullPath = %s\n", fullPath);

	if (!IsFileExist(fullPath))
	{
		PrintLog("file not found! Abort!()\n");

		abort();
		return;
	}

	bool found = false;
	uint64_t offset = 0;

	{
		uint64_t t1 = GetTimeInMs();

		for (uint64_t i = 0; i < 16 * 1024 * 1024; i += 4)
		{
			uint64_t v;
			lv1_read(i, 8, &v);

			if (v == 0x2F6F732F6C76325FULL)
			{
				found = true;
				offset = i - 4;

				break;
			}

			uint64_t t2 = GetTimeInMs();

			if ((t2 - t1) >= 2000)
			{
				lv2_beep_single();
				t1 = t2;
			}
		}
	}

	if (!found)
	{
		PrintLog("offset not found!, Abort!()\n");

		abort();
		return;
	}

	PrintLog("offset = 0x%lx\n", offset);

	{
		char fullPath[512];
		sprintf(fullPath, "/local_sys0/sys/%s", fileName);

		PrintLog("fullPath2 = %s\n", fullPath);

		size_t len = strlen(fullPath);
		lv1_write(offset, len + 1, fullPath);
	}

	if (type == LoadLv2KernelType_e::OtherOS_Fself)
	{
		PrintLog("Patching initial lpar size...\n");

		uint64_t initial_lpar_size_offset = offset + 0x127;
		PrintLog("initial_lpar_size_offset = 0x%lx\n", initial_lpar_size_offset);

		uint8_t v = 0x1B; // 128M
		lv1_write(initial_lpar_size_offset, 1, &v);
	}

	{
		// install our patch

		bool auth_lv2_offset_found = false;
		uint64_t auth_lv2_offset;

		{
			for (uint64_t i = 0; i < (16 * 1024 * 1024); i += 4)
			{
				uint64_t v[3];
				lv1_read(i, 24, v);

				if ((v[0] == 0xF821FF517C0802A6) && (v[1] == 0xFB610088FB810090) && (v[2] == 0xFBC100A07C7C1B78))
				{
					auth_lv2_offset_found = true;
					auth_lv2_offset = i;

					break;
				}
			}
		}

		if (!auth_lv2_offset_found)
		{
			PrintLog("auth_lv2_offset not found!\n");
		}
		else
		{
			PrintLog("auth_lv2_offset = 0x%lx\n", auth_lv2_offset);

			// max function size = 420

			if (type == LoadLv2KernelType_e::Fself || type == LoadLv2KernelType_e::OtherOS_Fself)
			{
				PrintLog("Writing our fself loader patch...\n");

				if (our_lv1_auth_lv2_hook_fself_do_size > 420)
				{
					PrintLog("function size too big!\n");

					abort();
					return;
				}

				PrintLog("Patch size = %lu\n", our_lv1_auth_lv2_hook_fself_do_size);

				lv1_write(auth_lv2_offset,
						  our_lv1_auth_lv2_hook_fself_do_size,
						  (void *)our_lv1_auth_lv2_hook_fself_do);
			}
		}
	}

	{
		bool mmap_size_offset_found = false;
		uint64_t mmap_size_offset;

		// 7C 7C 1B 78 7F C3 F3 78 7F 86 E3 78

		for (uint64_t i = 0; i < (16 * 1024 * 1024); i += 4)
		{
			uint32_t v[3];
			lv1_read(i, 12, v);

			if ((v[0] == 0x7C7C1B78) && (v[1] == 0x7FC3F378) && (v[2] == 0x7F86E378))
			{
				mmap_size_offset_found = true;
				mmap_size_offset = i;

				break;
			}
		}

		if (!mmap_size_offset_found)
		{
			PrintLog("mmap_size_offset not found!, Abort!()\n");

			abort();
			return;
		}

		PrintLog("mmap_size_offset = 0x%lx\n", mmap_size_offset);

		//uint32_t v = 0x3B831000;
		uint32_t v = 0x3B837000;
		lv1_write(mmap_size_offset, 4, &v);
	}

	UninstallOurHvcall();

	WaitInMs(1000);
	lv2_beep_triple();

	PrintLog("Booting lv2_kernel...\n");
	lv2_boot_lv2_kernel();
}