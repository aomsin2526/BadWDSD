#include "Include.h"

uint64_t SPU_CalcMMIOAddress(uint64_t spu_id, uint64_t offset)
{
    return 0x20000000000 + (0x80000 * spu_id) + offset;
}

uint64_t SPU_CalcMMIOAddress_LS(uint64_t spu_id, uint64_t offset)
{
    return SPU_CalcMMIOAddress(spu_id, offset);
}

uint64_t SPU_CalcMMIOAddress_PS(uint64_t spu_id, uint64_t offset)
{
    return SPU_CalcMMIOAddress(spu_id, offset) + 0x40000;
}

uint64_t SPU_CalcMMIOAddress_P2(uint64_t spu_id, uint64_t offset)
{
    return SPU_CalcMMIOAddress(spu_id, offset) + 0x60000;
}

uint64_t SPU_LS_Read64(uint64_t spu_id, uint64_t offset)
{
    return lv1_peek(SPU_CalcMMIOAddress_LS(spu_id, offset));
}

void SPU_LS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    lv1_poke(SPU_CalcMMIOAddress_LS(spu_id, offset), value);
}

uint32_t SPU_LS_Read32(uint64_t spu_id, uint64_t offset)
{
    return lv1_peek32(SPU_CalcMMIOAddress_LS(spu_id, offset));
}

void SPU_LS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    lv1_poke32(SPU_CalcMMIOAddress_LS(spu_id, offset), value);
}

uint64_t SPU_PS_Read64(uint64_t spu_id, uint64_t offset)
{
    return lv1_peek(SPU_CalcMMIOAddress_PS(spu_id, offset));
}

void SPU_PS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    lv1_poke(SPU_CalcMMIOAddress_PS(spu_id, offset), value);
}

uint32_t SPU_PS_Read32(uint64_t spu_id, uint64_t offset)
{
    return lv1_peek32(SPU_CalcMMIOAddress_PS(spu_id, offset));
}

void SPU_PS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    lv1_poke32(SPU_CalcMMIOAddress_PS(spu_id, offset), value);
}

uint64_t SPU_P2_Read64(uint64_t spu_id, uint64_t offset)
{
    return lv1_peek(SPU_CalcMMIOAddress_P2(spu_id, offset));
}

void SPU_P2_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    lv1_poke(SPU_CalcMMIOAddress_P2(spu_id, offset), value);
}

uint32_t SPU_P2_Read32(uint64_t spu_id, uint64_t offset)
{
    return lv1_peek32(SPU_CalcMMIOAddress_P2(spu_id, offset));
}

void SPU_P2_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    lv1_poke32(SPU_CalcMMIOAddress_P2(spu_id, offset), value);
}