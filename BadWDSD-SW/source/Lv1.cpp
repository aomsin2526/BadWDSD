#include "Include.h"

void CallLv1Function(CallLv1Function_Context_s *ctx)
{
	if (ctx == NULL)
	{
		PrintLog("ctx is NULL!\n");
		abort();
	}

	lv2syscall8(10, ctx->args[0], ctx->args[1], ctx->args[2], ctx->args[3], ctx->args[4], ctx->args[5], ctx->args[6], ctx->num);

	ctx->out[0] = p1;
	ctx->out[1] = p2;
	ctx->out[2] = p3;
	ctx->out[3] = p4;
	ctx->out[4] = p5;
	ctx->out[5] = p6;
	ctx->out[6] = p7;
	ctx->out[7] = p8;
}

uint64_t lv1_peek_114(uint64_t addr)
{
	if ((addr % 8) != 0)
	{
		PrintLog("lv1_peek_114 at addr = 0x%lx must be multiple of 8!!!\n", addr);

		abort();
		return 0;
	}

	uint64_t offset = (addr % 4096);
	uint64_t map_addr = (addr - offset);

	uint64_t lpar_addr;

	int32_t res;

	res = lv1_map_physical_address_region(map_addr, EXP_4KB, SIZE_4KB, &lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_map_physical_address_region failed!!!, res = %d, addr = 0x%lx, map_addr = 0x%lx\n", res, addr, map_addr);

		abort();
		return 0;
	}

	uint64_t dest_ea = 0x8000000015000000ul;
	map_lpar_to_lv2_ea(lpar_addr, dest_ea, SIZE_4KB, true, false);

	uint64_t v;

	v = lv2_peek(dest_ea + offset);

	res = lv1_unmap_physical_address_region(lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_unmap_physical_address_region failed!!!, res = %d\n", res);

		abort();
		return 0;
	}

	return v;
}

void lv1_poke_114(uint64_t addr, uint64_t val)
{
	if ((addr % 8) != 0)
	{
		PrintLog("lv1_poke_114 at addr = 0x%lx must be multiple of 8!!!\n", addr);

		abort();
		return;
	}

	uint64_t offset = (addr % 4096);
	uint64_t map_addr = (addr - offset);

	uint64_t lpar_addr;

	int32_t res;

	res = lv1_map_physical_address_region(map_addr, EXP_4KB, SIZE_4KB, &lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_map_physical_address_region failed!!!, res = %d\n", res);

		abort();
		return;
	}

	uint64_t dest_ea = 0x8000000015000000ul;
	map_lpar_to_lv2_ea(lpar_addr, dest_ea, SIZE_4KB, false, false);

	lv2_poke(dest_ea + offset, val);

	res = lv1_unmap_physical_address_region(lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_unmap_physical_address_region failed!!!, res = %d\n", res);

		abort();
		return;
	}

	uint64_t afterval = lv1_peek_114(addr);

	if (afterval != val)
	{
		PrintLog("lv1_poke failed!, addr = 0x%lx, val = 0x%lx, afterval = 0x%lx\n",
				 addr, val, afterval);

		abort();
		return;
	}
}

void lv1_read_114(uint64_t addr, uint64_t size, void *out_Buf)
{
	if (size == 0)
		return;

	uint64_t curOffset = 0;
	uint64_t left = size;

	uint64_t chunkSize = sizeof(uint64_t);

	uint8_t *outBuf = (uint8_t *)out_Buf;

	uint64_t zz = (addr % chunkSize);

	if (zz != 0)
	{
		uint64_t readSize = (chunkSize - zz);

		if (readSize > left)
			readSize = left;

		uint64_t a = (addr - zz);

		uint64_t v = lv1_peek_114(a);
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

		uint64_t v = lv1_peek_114(addr + curOffset);

		memcpy(&outBuf[curOffset], &v, readSize);

		curOffset += readSize;
		left -= readSize;
	}
}

void lv1_write_114(uint64_t addr, uint64_t size, const void *in_Buf)
{
	if (size == 0)
		return;

	uint64_t curOffset = 0;
	uint64_t left = size;

	uint64_t chunkSize = sizeof(uint64_t);

	const uint8_t *inBuf = (const uint8_t *)in_Buf;

	uint64_t zz = (addr % chunkSize);

	if (zz != 0)
	{
		uint64_t writeSize = (chunkSize - zz);

		if (writeSize > left)
			writeSize = left;

		uint64_t a = (addr - zz);

		uint64_t v = lv1_peek_114(a);
		uint8_t *vx = (uint8_t *)&v;

		memcpy(&vx[zz], &inBuf[curOffset], writeSize);

		lv1_poke_114(a, v);

		curOffset += writeSize;
		left -= writeSize;
	}

	while (1)
	{
		if (left == 0)
			break;

		uint64_t writeSize = (left > chunkSize) ? chunkSize : left;

		uint64_t v = lv1_peek_114(addr + curOffset);
		memcpy(&v, &inBuf[curOffset], writeSize);

		lv1_poke_114(addr + curOffset, v);

		curOffset += writeSize;
		left -= writeSize;
	}
}

uint64_t lv1_peek(uint64_t addr)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 34;

	ctx.args[0] = addr;

	CallLv1Function(&ctx);

	return ctx.out[0];
}

void lv1_poke(uint64_t addr, uint64_t val)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 35;

	ctx.args[0] = addr;
	ctx.args[1] = val;

	CallLv1Function(&ctx);

#if 0

	if (ctx.out[0] != 0)
	{
		PrintLog("lv1_poke failed!\n");

		abort();
		return;
	}

	uint64_t afterval = lv1_peek(addr);

	if (afterval != val)
	{
		PrintLog("lv1_poke failed!, addr = 0x%lx, val = 0x%lx, afterval = 0x%lx\n",
				 addr, val, afterval);

		abort();
		return;
	}

#endif
}

uint32_t lv1_peek32(uint64_t addr)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 37;

	ctx.args[0] = addr;

	CallLv1Function(&ctx);

	return (uint32_t)ctx.out[0];
}

void lv1_poke32(uint64_t addr, uint32_t val)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 38;

	ctx.args[0] = addr;
	ctx.args[1] = val;

	CallLv1Function(&ctx);
}

void lv1_read(uint64_t addr, uint64_t size, void *out_Buf)
{
	if (size == 0)
		return;

	uint64_t curOffset = 0;
	uint64_t left = size;

	uint64_t chunkSize = sizeof(uint64_t);

	uint8_t *outBuf = (uint8_t *)out_Buf;

	uint64_t zz = (addr % chunkSize);

	if (zz != 0)
	{
		uint64_t readSize = (chunkSize - zz);

		if (readSize > left)
			readSize = left;

		uint64_t a = (addr - zz);

		uint64_t v = lv1_peek(a);
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

		uint64_t v = lv1_peek(addr + curOffset);

		memcpy(&outBuf[curOffset], &v, readSize);

		curOffset += readSize;
		left -= readSize;
	}
}

void lv1_write(uint64_t addr, uint64_t size, const void *in_Buf)
{
	if (size == 0)
		return;

	uint64_t curOffset = 0;
	uint64_t left = size;

	uint64_t chunkSize = sizeof(uint64_t);

	const uint8_t *inBuf = (const uint8_t *)in_Buf;

	uint64_t zz = (addr % chunkSize);

	if (zz != 0)
	{
		uint64_t writeSize = (chunkSize - zz);

		if (writeSize > left)
			writeSize = left;

		uint64_t a = (addr - zz);

		uint64_t v = lv1_peek(a);
		uint8_t *vx = (uint8_t *)&v;

		memcpy(&vx[zz], &inBuf[curOffset], writeSize);

		lv1_poke(a, v);

		curOffset += writeSize;
		left -= writeSize;
	}

	while (1)
	{
		if (left == 0)
			break;

		uint64_t writeSize = (left > chunkSize) ? chunkSize : left;

		uint64_t v = lv1_peek(addr + curOffset);
		memcpy(&v, &inBuf[curOffset], writeSize);

		lv1_poke(addr + curOffset, v);

		curOffset += writeSize;
		left -= writeSize;
	}
}

int32_t lv1_allocate_memory(uint64_t size, uint64_t page_size_exp, uint64_t unknown, uint64_t flags, uint64_t *out_lpar_addr, uint64_t *muid)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 0;

	ctx.args[0] = size;
	ctx.args[1] = page_size_exp;
	ctx.args[2] = unknown;
	ctx.args[3] = flags;

	CallLv1Function(&ctx);

	if (out_lpar_addr)
		*out_lpar_addr = ctx.out[1];

	if (muid)
		*muid = ctx.out[2];

	return (int32_t)ctx.out[0];
}

int32_t lv1_release_memory(uint64_t lpar_addr)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 13;

	ctx.args[0] = lpar_addr;

	CallLv1Function(&ctx);

	return (int32_t)ctx.out[0];
}

int32_t lv1_write_htab_entry(uint64_t vas_id, uint64_t slot, uint64_t va, uint64_t pa)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 1;

	ctx.args[0] = vas_id;
	ctx.args[1] = slot;
	ctx.args[2] = va;
	ctx.args[3] = pa;

	CallLv1Function(&ctx);

	return (int32_t)ctx.out[0];
}

int32_t lv1_insert_htab_entry(uint64_t vas_id, uint64_t hpte_group, uint64_t hpte_v, uint64_t hpte_r, uint64_t bolted, uint64_t flags, uint64_t *out_hpte_index, uint64_t *out_hpte_evicted_v, uint64_t *out_hpte_evicted_r)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 158;

	ctx.args[0] = vas_id;
	ctx.args[1] = hpte_group;
	ctx.args[2] = hpte_v;
	ctx.args[3] = hpte_r;
	ctx.args[4] = bolted;
	ctx.args[5] = flags;

	CallLv1Function(&ctx);

	if (out_hpte_index)
		*out_hpte_index = ctx.out[1];

	if (out_hpte_evicted_v)
		*out_hpte_evicted_v = ctx.out[2];

	if (out_hpte_evicted_r)
		*out_hpte_evicted_r = ctx.out[3];

	return (int32_t)ctx.out[0];
}

int32_t lv1_map_physical_address_region(uint64_t phys_addr, uint64_t page_size, uint64_t size, uint64_t *out_lpar_addr)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 114;

	ctx.args[0] = phys_addr;
	ctx.args[1] = page_size;
	ctx.args[2] = size;

	CallLv1Function(&ctx);

	if (out_lpar_addr)
		*out_lpar_addr = ctx.out[1];

	return (int32_t)ctx.out[0];
}

int32_t lv1_unmap_physical_address_region(uint64_t lpar_addr)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 115;

	ctx.args[0] = lpar_addr;

	CallLv1Function(&ctx);

	return (int32_t)ctx.out[0];
}

int32_t lv1_construct_virtual_address_space(uint64_t htab_size, uint64_t number_of_sizes, uint64_t page_sizes, uint64_t *vas_id, uint64_t *act_htab_size)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 2;

	ctx.args[0] = htab_size;
	ctx.args[1] = number_of_sizes;
	ctx.args[2] = page_sizes;

	CallLv1Function(&ctx);

	if (vas_id)
		*vas_id = ctx.out[1];

	if (act_htab_size)
		*act_htab_size = ctx.out[2];

	return (int32_t)ctx.out[0];
}

int32_t lv1_destruct_virtual_address_space(uint64_t vas_id)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 10;

	ctx.args[0] = vas_id;

	CallLv1Function(&ctx);

	return (int32_t)ctx.out[0];
}

int32_t lv1_select_virtual_address_space(uint64_t vas_id)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 7;

	ctx.args[0] = vas_id;

	CallLv1Function(&ctx);

	return (int32_t)ctx.out[0];
}

int32_t lv1_map_htab(uint64_t vas_id, uint64_t *htab_addr)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 122;

	ctx.args[0] = vas_id;

	CallLv1Function(&ctx);

	if (htab_addr)
		*htab_addr = ctx.out[1];

	return (int32_t)ctx.out[0];
}

int32_t lv1_unmap_htab(uint64_t htab_addr)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 123;

	ctx.args[0] = htab_addr;

	CallLv1Function(&ctx);

	return (int32_t)ctx.out[0];
}

int32_t lv1_query_logical_partition_address_region_info(uint64_t lpar_addr,
														uint64_t *start_address, uint64_t *size, uint64_t *access_right, uint64_t *max_page_size, uint64_t *flags)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 6;

	ctx.args[0] = lpar_addr;

	CallLv1Function(&ctx);

	if (start_address)
		*start_address = ctx.out[1];

	if (size)
		*size = ctx.out[2];

	if (access_right)
		*access_right = ctx.out[3];

	if (max_page_size)
		*max_page_size = ctx.out[4];

	if (flags)
		*flags = ctx.out[5];

	return (int32_t)ctx.out[0];
}

int32_t lv1_get_virtual_address_space_id_of_ppe(uint64_t ppe_id, uint64_t *vas_id)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 4;

	ctx.args[0] = ppe_id;

	CallLv1Function(&ctx);

	if (vas_id)
		*vas_id = ctx.out[1];

	return (int32_t)ctx.out[0];
}

int32_t lv1_get_logical_ppe_id(uint64_t *ppe_id)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 69;

	CallLv1Function(&ctx);

	if (ppe_id)
		*ppe_id = ctx.out[1];

	return (int32_t)ctx.out[0];
}

uint64_t lv1_repository_string(const char *str)
{
	uint64_t ret = 0;
	strncpy((char *)&ret, str, sizeof(ret));
	return (ret);
}

int32_t lv1_modify_repository_node_value(uint64_t lpar_id, uint64_t n1, uint64_t n2, uint64_t n3, uint64_t n4, uint64_t v1, uint64_t v2)
{
	CallLv1Function_Context_s ctx;

	ctx.num = 92;

	ctx.args[0] = lpar_id;

	ctx.args[1] = n1;
	ctx.args[2] = n2;
	ctx.args[3] = n3;
	ctx.args[4] = n4;

	ctx.args[5] = v1;
	ctx.args[6] = v2;

	CallLv1Function(&ctx);

	return (int32_t)ctx.out[0];
}