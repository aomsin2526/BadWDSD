#include "Include.h"

uint64_t _our_hvcall_table_addr;
uint64_t _our_hvcall_lpar_addr;

uint64_t FindHvcallTable()
{
	PrintLog("FindHvcallTable()\n");

	// find invalid handler addr

	bool invalid_handler_addr_found = false;
	uint64_t invalid_handler_addr = 0;

	for (uint64_t i = 0; i < (16 * 1024 * 1024); i += 4)
	{
		uint64_t v[2];
		lv1_read_114(i, 16, v);

		if ((v[0] == 0x386000006463ffff) && (v[1] == 0x6063ffec4e800020))
		{
			invalid_handler_addr_found = true;
			invalid_handler_addr = i;

			break;
		}
	}

	if (!invalid_handler_addr_found)
	{
		PrintLog("invalid_handler_addr not found!, Abort!()\n");

		abort();
		return 0;
	}

	PrintLog("invalid_handler_addr = 0x%lx\n", invalid_handler_addr);

	// find table

	bool table_addr_found = false;
	uint64_t table_addr = 0;

	for (uint64_t i = 0; i < (16 * 1024 * 1024); i += 8)
	{
		uint64_t v[4];
		lv1_read_114(i, 32, v);

		if ((v[0] == invalid_handler_addr) &&
			(v[1] == invalid_handler_addr) &&
			(v[2] != invalid_handler_addr) &&
			(v[3] == invalid_handler_addr))
		{
			table_addr_found = true;
			table_addr = (i - (22 * 8));

			break;
		}
	}

	if (!table_addr_found)
	{
		PrintLog("table_addr not found!, Abort!()\n");

		abort();
		return 0;
	}

	PrintLog("table_addr = 0x%lx\n", table_addr);

	return table_addr;
}

bool IsOurHvcallInstalled()
{
	// test poke

	{
		CallLv1Function_Context_s ctx;

		ctx.num = 35;

		ctx.args[0] = 0x0; // addr
		ctx.args[1] = 0;   // val

		CallLv1Function(&ctx);

		if (ctx.out[0] != 0)
			return false;
	}

	// test exec

	CallLv1ExecEa_Context_s ctx;

	ctx.ea = (uint64_t)our_lv1_exec_test_do;
	ctx.size = our_lv1_exec_test_do_size;

	CallLv1ExecEa(&ctx);

	if (ctx.out[0] != 0x39)
	{
		PrintLog("test exec failed!\n");

		abort();
		return false;
	}

	return true;
}

void InstallOurHvcall()
{
	PrintLog("InstallOurHvcall()\n");

	int32_t res;

	if (IsOurHvcallInstalled())
	{
		PrintLog("our hvcall already installed, skip\n");
		return;
	}

	bool table_addr_found = false;
	uint64_t table_addr = 0;

	if (fwVersion >= 4.70)
	{
		table_addr = 0x372D08;
		table_addr_found = true;
	}
	else
	{
		table_addr = FindHvcallTable();
		table_addr_found = true;
	}

	if (!table_addr_found)
	{
		PrintLog("table_addr not found!, Abort!()\n");

		abort();
		return;
	}

	PrintLog("table_addr = 0x%lx\n", table_addr);
	_our_hvcall_table_addr = table_addr;

	uint64_t lpar_addr = 0;
	uint64_t muid;

	res = lv1_allocate_memory(SIZE_4KB, EXP_4KB, 0, 0, &lpar_addr, &muid);

	if (res != 0)
	{
		PrintLog("lv1_allocate_memory failed!, res = %d\n", res);

		abort();
		return;
	}

	PrintLog("lpar_addr = 0x%lx\n", lpar_addr);
	_our_hvcall_lpar_addr = lpar_addr;

	uint64_t ra = htab_ra_from_lpar(lpar_addr);
	PrintLog("ra = 0x%lx\n", ra);

	// uint64_t ea = 0x8000000016000000ul;
	// map_lpar_to_lv2_ea(lpar_addr, ea, SIZE_4KB, false, true);

	PrintLog("Writing our code and to lv1 memory and hvcall table...\n");

	{
		uint64_t offset = ra;

		// 34 = peek

		{
			lv1_write_114(table_addr + (34 * 8), 8, &offset);

			// E8 63 00 00 4E 80 00 20

			uint64_t val = 0xE86300004E800020;
			lv1_write_114(offset, 8, &val);
			offset += 8;
		}

		// 35 = poke

		{
			lv1_write_114(table_addr + (35 * 8), 8, &offset);

			{
				// F8 83 00 00 38 60 00 00

				uint64_t val = 0xF883000038600000;
				lv1_write_114(offset, 8, &val);
				offset += 8;
			}

			{
				// 4E 80 00 20

				uint32_t val = 0x4E800020;
				lv1_write_114(offset, 4, &val);
				offset += 4;
			}
		}

#if 0

		// 36 = exec

		{
			lv1_write_114(table_addr + (36 * 8), 8, &offset);

			{
				// 38 21 FF F0 7C 08 02 A6

				uint64_t val = 0x3821FFF07C0802A6;
				lv1_write_114(offset, 8, &val);
				offset += 8;
			}

			{
				// F8 01 00 00 7D 29 03 A6

				uint64_t val = 0xF80100007D2903A6;
				lv1_write_114(offset, 8, &val);
				offset += 8;
			}

			{
				// 4E 80 04 21 E8 01 00 00

				uint64_t val = 0x4E800421E8010000;
				lv1_write_114(offset, 8, &val);
				offset += 8;
			}

			{
				// 7C 08 03 A6 38 21 00 10

				uint64_t val = 0x7C0803A638210010;
				lv1_write_114(offset, 8, &val);
				offset += 8;
			}

			{
				// 4E 80 00 20

				uint32_t val = 0x4E800020;
				lv1_write_114(offset, 4, &val);
				offset += 4;
			}
		}

#else

		lv1_write_114(table_addr + (36 * 8), 8, &offset);

		lv1_write_114(offset, our_lv1_exec_do_size, (void *)our_lv1_exec_do);
		offset += our_lv1_exec_do_size;

#endif

		// 37 = peek32

		{
			lv1_write_114(table_addr + (37 * 8), 8, &offset);

			// 80 63 00 00 4E 80 00 20

			uint64_t val = 0x806300004E800020;
			lv1_write_114(offset, 8, &val);
			offset += 8;
		}

		// 38 = poke32

		{
			lv1_write_114(table_addr + (38 * 8), 8, &offset);

			{
				// 90 83 00 00 38 60 00 00

				uint64_t val = 0x9083000038600000;
				lv1_write_114(offset, 8, &val);
				offset += 8;
			}

			{
				// 4E 80 00 20

				uint32_t val = 0x4E800020;
				lv1_write_114(offset, 4, &val);
				offset += 4;
			}
		}
	}

	PrintLog("write done.\n");

	eieio();

	if (!IsOurHvcallInstalled())
	{
		PrintLog("install our hvcall failed!\n");

		abort();
		return;
	}

	lv1_test_puts();

	PrintLog("InstallOurHvcall() done.\n");
}

void UninstallOurHvcall()
{
	PrintLog("UninstallOurHvcall()\n");

	PrintLog("our_hvcall_table_addr = 0x%lx\n", _our_hvcall_table_addr);
	PrintLog("our_hvcall_lpar_addr = 0x%lx\n", _our_hvcall_lpar_addr);

	{
		uint64_t invalid_handler_addr;
		lv1_read_114(_our_hvcall_table_addr + (37 * 8), 8, &invalid_handler_addr);
		PrintLog("invalid_handler_addr = 0x%lx\n", invalid_handler_addr);

		lv1_write_114(_our_hvcall_table_addr + (34 * 8), 8, &invalid_handler_addr);
		lv1_write_114(_our_hvcall_table_addr + (35 * 8), 8, &invalid_handler_addr);
		lv1_write_114(_our_hvcall_table_addr + (36 * 8), 8, &invalid_handler_addr);
	}

	int32_t res = lv1_release_memory(_our_hvcall_lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_release_memory failed!, res = %d\n", res);

		abort();
		return;
	}

	PrintLog("UninstallOurHvcall() done.\n");
}

void CallLv1Exec(CallLv1Exec_Context_s *ctx)
{
	CallLv1Function_Context_s ctxx;

	ctxx.num = 36;

	ctxx.args[0] = ctx->args[0];
	ctxx.args[1] = ctx->args[1];
	ctxx.args[2] = ctx->args[2];
	ctxx.args[3] = ctx->args[3];
	ctxx.args[4] = ctx->args[4];
	ctxx.args[5] = ctx->args[5];

	ctxx.args[6] = ctx->ra;

	CallLv1Function(&ctxx);

	ctx->out[0] = ctxx.out[0];
	ctx->out[1] = ctxx.out[1];
	ctx->out[2] = ctxx.out[2];
	ctx->out[3] = ctxx.out[3];
	ctx->out[4] = ctxx.out[4];
	ctx->out[5] = ctxx.out[5];
	ctx->out[6] = ctxx.out[6];
	ctx->out[7] = ctxx.out[7];
}

void CallLv1ExecEa(CallLv1ExecEa_Context_s *ctx)
{
	if (ctx->size > 4096)
	{
		PrintLog("function size too big!\n");

		abort();
		return;
	}

	// PrintLog("ctx->ea = 0x%lx, ctx->size = %lu\n", ctx->ea, ctx->size);

	CallLv1Function_Context_s ctxx;

	ctxx.num = 36;

	ctxx.args[0] = ctx->args[0];
	ctxx.args[1] = ctx->args[1];
	ctxx.args[2] = ctx->args[2];
	ctxx.args[3] = ctx->args[3];
	ctxx.args[4] = ctx->args[4];
	ctxx.args[5] = ctx->args[5];

	int32_t res;

	uint64_t lpar_addr = 0;
	uint64_t muid;

	res = lv1_allocate_memory(SIZE_4KB, EXP_4KB, 0, 0, &lpar_addr, &muid);

	if (res != 0)
	{
		PrintLog("lv1_allocate_memory failed!, res = %d\n", res);

		abort();
		return;
	}

	// PrintLog("lpar_addr = 0x%lx\n", lpar_addr);

	uint64_t ra = htab_ra_from_lpar(lpar_addr);
	PrintLog("ra = 0x%lx\n", ra);

	lv1_write(ra, ctx->size, (void *)ctx->ea);

	ctxx.args[6] = ra;

	uint64_t t1 = GetTimeInMs();
	CallLv1Function(&ctxx);
	uint64_t t2 = GetTimeInMs();

	PrintLog("delta = %lums\n", (t2 - t1));

	res = lv1_release_memory(lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_release_memory failed!, res = %d\n", res);

		abort();
		return;
	}

	ctx->out[0] = ctxx.out[0];
	ctx->out[1] = ctxx.out[1];
	ctx->out[2] = ctxx.out[2];
	ctx->out[3] = ctxx.out[3];
	ctx->out[4] = ctxx.out[4];
	ctx->out[5] = ctxx.out[5];
	ctx->out[6] = ctxx.out[6];
	ctx->out[7] = ctxx.out[7];
}

void lv1_test_puts()
{
	PrintLog("lv1_test_puts()\n");

	CallLv1ExecEa_Context_s ctx;

	ctx.ea = (uint64_t)our_lv1_test_puts_do;
	ctx.size = our_lv1_test_puts_do_size;

	CallLv1ExecEa(&ctx);

	PrintLog("lv1_test_puts() done.\n");
}

void lv1_apply_rsx_clock(uint64_t core_mul, uint64_t mem_mul)
{
	PrintLog("lv1_apply_rsx_clock()\n");

	CallLv1ExecEa_Context_s ctx;

	ctx.ea = (uint64_t)our_lv1_apply_rsx_clock_do;
	ctx.size = our_lv1_apply_rsx_clock_do_size;

	ctx.args[0] = core_mul;
	ctx.args[1] = mem_mul;

	CallLv1ExecEa(&ctx);

	PrintLog("lv1_apply_rsx_clock() done. res = 0x%lx\n", (uint64_t)ctx.out[0]);
}

void BadWDSD_Stage1_Test()
{
	PrintLog("BadWDSD_Stage1_Test()\n");

	uint64_t size = (__stop_BadWDSD_Stage1_Test_Do_Section - __start_BadWDSD_Stage1_Test_Do_Section);
	// uint64_t size = 40;
	PrintLog("size = %lu\n", size);

	CallLv1ExecEa_Context_s ctx;

	uint64_t xxx = *((uint64_t *)__start_BadWDSD_Stage1_Test_Do_Section);
	PrintLog("xxx = 0x%lx\n", xxx);

	ctx.ea = (uint64_t)__start_BadWDSD_Stage1_Test_Do_Section;
	ctx.size = size;

	CallLv1ExecEa(&ctx);

	PrintLog("BadWDSD_Stage1_Test() done. r3 = 0x%lx\n", (uint64_t)ctx.out[0]);
}

void BadWDSD_Stage1_Bin_Test()
{
	PrintLog("BadWDSD_Stage1_Bin_Test()\n");

	FILE *f = fopen("/app_home/Stage1.bin", "rb");

	if (f == NULL)
	{
		PrintLog("Stage1.bin not found!\n");

		abort();
		return;
	}

	size_t size = GetFileSize(f);
	PrintLog("size = %lu\n", size);

	void *code = malloc(size);
	fread(code, 1, size, f);

	fclose(f);

	PrintLog("code = 0x%lx\n", (uint64_t)code);

	CallLv1ExecEa_Context_s ctx;

	ctx.ea = (uint64_t)code;
	ctx.size = size;

	CallLv1ExecEa(&ctx);

	free(code);

	PrintLog("BadWDSD_Stage1_Bin_Test() done. r3 = 0x%lx\n", (uint64_t)ctx.out[0]);
}

void BadWDSD_Stage1_Bin_Flash_Test(bool exec)
{
	PrintLog("BadWDSD_Stage1_Bin_Flash_Test()\n");

	if (!FlashIsNor())
	{
		PrintLog("Flash is not nor!!!\n");

		abort();
		return;
	}

	FILE *f = fopen("/app_home/Stage1.bin", "rb");

	if (f == NULL)
	{
		PrintLog("Stage1.bin not found!\n");

		abort();
		return;
	}

	size_t size = GetFileSize(f);
	PrintLog("size = %lu\n", size);

	void *code = malloc(size);
	fread(code, 1, size, f);

	fclose(f);

	PrintLog("code = 0x%lx\n", (uint64_t)code);

	if (size > (32 * 1024))
	{
		PrintLog("size is too big!!!\n");

		abort();
		return;
	}

	PrintLog("Writing to flash...\n");
	// lv1_write(0x2401F031000, size, code);
	NorWrite(0x31000, code, size);

	{
		PrintLog("0x%lx\n", lv1_peek(0x2401F000200));
		PrintLog("0x%lx\n", lv1_peek(0x2401F031000));
	}

	CallLv1Exec_Context_s ctx;
	ctx.ra = 0x2401F031000;

	if (exec)
	{
		PrintLog("Execute...\n");
		CallLv1Exec(&ctx);
	}

	free(code);
	PrintLog("BadWDSD_Stage1_Bin_Flash_Test() done. r3 = 0x%lx\n", (uint64_t)ctx.out[0]);
}