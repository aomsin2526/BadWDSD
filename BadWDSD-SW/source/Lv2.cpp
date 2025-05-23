#include "Include.h"

uint64_t lv2_peek(uint64_t addr)
{
	lv2syscall1(6, addr);

	return_to_user_prog(uint64_t);
}

void lv2_poke(uint64_t addr, uint64_t val)
{
	lv2syscall2(7, addr, val);

	uint64_t afterval = lv2_peek(addr);

	if (afterval != val)
	{
		PrintLog("lv2_poke failed!, addr = 0x%lx, val = 0x%lx, afterval = 0x%lx\n",
			addr, val, afterval);

		abort();
		return;
	}
}

void lv2_read(uint64_t addr, uint64_t size, void* out_Buf)
{
	if (size == 0)
		return;

	uint64_t curOffset = 0;
	uint64_t left = size;

	uint64_t chunkSize = sizeof(uint64_t);

	uint8_t* outBuf = (uint8_t*)out_Buf;

	uint64_t zz = (addr % chunkSize);

	if (zz != 0)
	{
		uint64_t readSize = (chunkSize - zz);

		if (readSize > left)
			readSize = left;

		uint64_t a = (addr - zz);

		uint64_t v = lv2_peek(a);
		uint8_t *vx = (uint8_t *)&v;

		memcpy(&outBuf[curOffset], &vx[zz], readSize);

		curOffset += readSize;
		left -= readSize;
	}

	while (1)
	{
		if (left == 0)
			break;

		uint64_t readSize = (left > chunkSize) ? chunkSize : left;

		uint64_t v = lv2_peek(addr + curOffset);

		memcpy(&outBuf[curOffset], &v, readSize);

		curOffset += readSize;
		left -= readSize;
	}
}

void lv2_write(uint64_t addr, uint64_t size, const void* in_Buf)
{
	if (size == 0)
		return;

	uint64_t curOffset = 0;
	uint64_t left = size;

	uint64_t chunkSize = sizeof(uint64_t);

	const uint8_t* inBuf = (const uint8_t* )in_Buf;

	uint64_t zz = (addr % chunkSize);

	if (zz != 0)
	{
		uint64_t writeSize = (chunkSize - zz);

		if (writeSize > left)
			writeSize = left;

		uint64_t a = (addr - zz);

		uint64_t v = lv2_peek(a);
		uint8_t *vx = (uint8_t *)&v;

		memcpy(&vx[zz], &inBuf[curOffset], writeSize);

		lv2_poke(a, v);

		curOffset += writeSize;
		left -= writeSize;
	}

	while (1)
	{
		if (left == 0)
			break;

		uint64_t writeSize = (left > chunkSize) ? chunkSize : left;

		uint64_t v = lv2_peek(addr + curOffset);
		memcpy(&v, &inBuf[curOffset], writeSize);

		lv2_poke(addr + curOffset, v);

		curOffset += writeSize;
		left -= writeSize;
	}
}

uint64_t lv2_lv1_peek(uint64_t addr)
{
	lv2syscall1(11, addr);

	return_to_user_prog(uint64_t);
}

void lv2_lv1_poke(uint64_t addr, uint64_t val)
{
	lv2syscall2(9, addr, val);

	uint64_t afterval = lv2_lv1_peek(addr);

	if (afterval != val)
	{
		PrintLog("lv2_lv1_poke failed!, addr = 0x%lx, val = 0x%lx, afterval = 0x%lx\n",
			addr, val, afterval);

		abort();
		return;
	}
}

void lv2_lv1_read(uint64_t addr, uint64_t size, void* out_Buf)
{
	if (size == 0)
		return;

	uint64_t curOffset = 0;
	uint64_t left = size;

	uint64_t chunkSize = sizeof(uint64_t);

	uint8_t* outBuf = (uint8_t*)out_Buf;

	uint64_t zz = (addr % chunkSize);

	if (zz != 0)
	{
		uint64_t readSize = (chunkSize - zz);

		if (readSize > left)
			readSize = left;

		uint64_t a = (addr - zz);

		uint64_t v = lv2_lv1_peek(a);
		uint8_t *vx = (uint8_t *)&v;

		memcpy(&outBuf[curOffset], &vx[zz], readSize);

		curOffset += readSize;
		left -= readSize;
	}

	while (1)
	{
		if (left == 0)
			break;

		uint64_t readSize = (left > chunkSize) ? chunkSize : left;

		uint64_t v = lv2_lv1_peek(addr + curOffset);

		memcpy(&outBuf[curOffset], &v, readSize);

		curOffset += readSize;
		left -= readSize;
	}
}

void lv2_lv1_write(uint64_t addr, uint64_t size, const void* in_Buf)
{
	if (size == 0)
		return;

	uint64_t curOffset = 0;
	uint64_t left = size;

	uint64_t chunkSize = sizeof(uint64_t);

	const uint8_t* inBuf = (const uint8_t* )in_Buf;

	uint64_t zz = (addr % chunkSize);

	if (zz != 0)
	{
		uint64_t writeSize = (chunkSize - zz);

		if (writeSize > left)
			writeSize = left;

		uint64_t a = (addr - zz);

		uint64_t v = lv2_lv1_peek(a);
		uint8_t *vx = (uint8_t *)&v;

		memcpy(&vx[zz], &inBuf[curOffset], writeSize);

		lv2_lv1_poke(a, v);

		curOffset += writeSize;
		left -= writeSize;
	}

	while (1)
	{
		if (left == 0)
			break;

		uint64_t writeSize = (left > chunkSize) ? chunkSize : left;

		uint64_t v = lv2_lv1_peek(addr + curOffset);
		memcpy(&v, &inBuf[curOffset], writeSize);

		lv2_lv1_poke(addr + curOffset, v);

		curOffset += writeSize;
		left -= writeSize;
	}
}

void lv2_beep_single()
{
	lv2syscall3(392, 0x1004, 0x4, 0x6);
}

void lv2_beep_triple()
{
	lv2syscall3(392, 0x1004, 0xa, 0x1B6);
}

void lv2_beep_long()
{
	lv2syscall3(392, 0x1004, 0xa, 0xFFF);
}

int32_t lv2_storage_get_cache_of_flash_ext_flag(uint8_t* out_flag)
{
	lv2syscall1(874, (uint64_t)out_flag);

	return_to_user_prog(int32_t);
}

int32_t lv2_storage_get_device_info(uint64_t dev_id, lv2_storage_device_info* info)
{
	lv2syscall2(609, dev_id, (uint64_t) info);

	return_to_user_prog(int32_t);
}

int32_t lv2_storage_open(uint64_t dev_id, uint32_t* dev_handle)
{
	lv2syscall4(600, dev_id, 0, (uint64_t)dev_handle, 0);

	return_to_user_prog(int32_t);
}

int32_t lv2_storage_close(uint32_t dev_handle)
{
	lv2syscall1(601, dev_handle);

	return_to_user_prog(int32_t);
}

int32_t lv2_storage_read(uint32_t dev_handle, uint64_t unknown1, uint64_t start_sector, uint64_t sector_count,
	const void *buf, uint32_t *unknown2, uint64_t flags)
{
	lv2syscall7(602, dev_handle, unknown1, start_sector, sector_count,
		(uint64_t )buf, (uint64_t)unknown2, flags);

	return_to_user_prog(int32_t);
}

int32_t lv2_storage_write(uint32_t dev_handle, uint64_t unknown1, uint64_t start_sector, uint64_t sector_count,
	const void *buf, uint32_t *unknown2, uint64_t flags)
{
	lv2syscall7(603, dev_handle, unknown1, start_sector, sector_count,
		(uint64_t )buf, (uint64_t)unknown2, flags);

	return_to_user_prog(int32_t);
}

int32_t lv2_dbg_get_console_type(uint64_t* out_type)
{
	lv2syscall1(985, (uint64_t)out_type);

	return_to_user_prog(int32_t);
}

int32_t lv2_sm_shutdown(uint16_t op, const void* lpar_parameter, uint64_t parameter_size)
{
	lv2syscall3(379, (uint64_t)op, (uint64_t)lpar_parameter, (uint64_t)parameter_size);

	return_to_user_prog(int32_t);
}

int32_t lv2_um_read_eeprom(uint32_t offset, uint8_t* outValue)
{
	lv2syscall3(863, 0x600B, (uint64_t)offset, (uint64_t)outValue);

	return_to_user_prog(int32_t);
}

int32_t lv2_um_write_eeprom(uint32_t offset, uint8_t inValue)
{
	lv2syscall3(863, 0x600C, (uint64_t)offset, (uint64_t)inValue);

	return_to_user_prog(int32_t);
}

void lv2_shutdown()
{
	lv2_sm_shutdown(0x100, NULL, 0);

	PrintLog("lv2_shutdown should not be here!!!\n");
	abort();
}

void lv2_reboot_soft()
{
	lv2_sm_shutdown(0x200, NULL, 0);
	
	PrintLog("lv2_reboot_soft should not be here!!!\n");
	abort();
}

void lv2_reboot_hard()
{
	lv2_sm_shutdown(0x1200, NULL, 0);
	
	PrintLog("lv2_reboot_hard should not be here!!!\n");
	abort();
}

void lv2_boot_otheros()
{
	lv2_sm_shutdown(0x8201, NULL, 0);
	
	PrintLog("lv2_boot_otheros should not be here!!!\n");
	abort();
}

void lv2_boot_lv2_kernel()
{
	lv2_sm_shutdown(0x8201, NULL, 0);
	
	PrintLog("lv2_boot_lv2_kernel should not be here!!!\n");
	abort();
}

void CallLv2Function(CallLv2Function_Context_s* ctx)
{
	if (ctx == NULL)
	{
		PrintLog("ctx is NULL!\n");
		abort();
	}

	lv2syscall8(15, ctx->args[0], ctx->args[1], ctx->args[2], ctx->args[3], ctx->args[4], ctx->args[5], ctx->args[6], ctx->addr);

	ctx->out[0] = p1;
	ctx->out[1] = p2;
	ctx->out[2] = p3;
	ctx->out[3] = p4;
	ctx->out[4] = p5;
	ctx->out[5] = p6;
	ctx->out[6] = p7;
	ctx->out[7] = p8;
}

uint64_t lv2_mfsprg0()
{
    CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_mfsprg0_do;

    CallLv2Function(&ctx);

    return ctx.out[1];
}

void lv2_slbie(uint64_t esid)
{
    CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_slbie_do;

    ctx.args[0] = esid;

    CallLv2Function(&ctx);
}

void lv2_slbmte(uint64_t vsid, uint64_t esid)
{
    CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_slbmte_do;

    ctx.args[0] = vsid;
    ctx.args[1] = esid;

    CallLv2Function(&ctx);
}

void lv2_slbmfev(uint64_t slb_index, uint64_t* out_vsid)
{
    CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_slbmfev_do;

    ctx.args[0] = slb_index;

    CallLv2Function(&ctx);

    if (out_vsid != NULL)
        *out_vsid = ctx.out[1];
}

void lv2_slbmfee(uint64_t slb_index, uint64_t* out_esid)
{
    CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_slbmfee_do;

    ctx.args[0] = slb_index;

    CallLv2Function(&ctx);

    if (out_esid != NULL)
        *out_esid = ctx.out[1];
}

void lv2_disable_interrupt()
{
    CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_disable_interrupt_do;

    CallLv2Function(&ctx);
}

void lv2_enable_interrupt()
{
    CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_enable_interrupt_do;

    CallLv2Function(&ctx);
}

void lv2_disable_enable_interrupt()
{
    CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_disable_enable_interrupt_do;

    CallLv2Function(&ctx);
}

void lv2_write_lv1_rw_htabe(uint64_t old_vas_id, uint64_t new_vas_id)
{
    CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_write_lv1_rw_htabe_do;

    ctx.args[1] = old_vas_id;
    ctx.args[2] = new_vas_id;

    CallLv2Function(&ctx);

    PrintLog("lv2_write_lv1_rw_htabe() res1 = %d, res2 = %d\n", (int32_t)ctx.out[1], (int32_t)ctx.out[2]);
}

void lv2_get_pir(uint64_t* out_pir)
{
    CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_get_pir_do;

    CallLv2Function(&ctx);

    if (out_pir != NULL)
        *out_pir = ctx.out[1];
}

void lv2_glitcher_test(uint64_t addr, uint64_t size, uint64_t* out_WriteCount)
{
	CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_glitcher_test_do;

	ctx.args[0] = addr;
    ctx.args[1] = size;

    CallLv2Function(&ctx);

    if (out_WriteCount != NULL)
        *out_WriteCount = ctx.out[1];
}

int32_t lv2_lv1_release_memory_intr(uint64_t lpar_addr)
{
	CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_lv1_release_memory_intr_do;

	ctx.args[0] = lpar_addr;

    CallLv2Function(&ctx);

	return (int32_t)ctx.out[0];
}

void lv2_dcbi(uint64_t ea)
{
    CallLv2Function_Context_s ctx;
    ctx.addr = (uint64_t)lv2_dcbi_do;

    ctx.args[0] = ea;

    CallLv2Function(&ctx);
}