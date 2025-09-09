#include <spu_intrinsics.h>
#include <spu_mfcio.h>

#include <stdint.h>
#include <stddef.h>
//#include <string.h>

#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"

// typedef char int8_t;
// typedef unsigned char uint8_t;

// typedef short int16_t;
// typedef unsigned short uint16_t;

// typedef int int32_t;
// typedef unsigned int uint32_t;

// typedef long long int64_t;
// typedef unsigned long long uint64_t;

// typedef uint32_t size_t;

// typedef uint32_t uintptr_t;

// #define NULL 0

#define sync() asm volatile("sync")
#define stop(...) asm volatile("stop %0" ::"i"(__VA_ARGS__) :)

uint8_t IsPow2(uint64_t x)
{
    return ((x & (x - 1)) == 0) ? 1 : 0;
}

#if 1

void memset(void *buf, uint8_t v, uint32_t count)
{
    if ((((uint32_t)buf % 8) == 0) && ((count % 8) == 0))
    {
        uint64_t *buff = (uint64_t *)buf;

        for (uint32_t i = 0; i < (count / 8); ++i)
            buff[i] = v;
    }
    else if ((((uint32_t)buf % 4) == 0) && ((count % 4) == 0))
    {
        uint32_t *buff = (uint32_t *)buf;

        for (uint32_t i = 0; i < (count / 4); ++i)
            buff[i] = v;
    }
    else if ((((uint32_t)buf % 2) == 0) && ((count % 2) == 0))
    {
        uint16_t *buff = (uint16_t *)buf;

        for (uint32_t i = 0; i < (count / 2); ++i)
            buff[i] = v;
    }
    else
    {
        uint8_t *buff = (uint8_t *)buf;

        for (uint32_t i = 0; i < count; ++i)
            buff[i] = v;
    }
}

void memcpy(void *dest, const void *src, uint32_t count)
{
    if ((((uint32_t)dest % 8) == 0) && (((uint32_t)src % 8) == 0) && ((count % 8) == 0))
    {
        uint64_t *destt = (uint64_t *)dest;
        const uint64_t *srcc = (const uint64_t *)src;

        for (uint32_t i = 0; i < (count / 8); ++i)
            destt[i] = srcc[i];
    }
    else if ((((uint32_t)dest % 4) == 0) && (((uint32_t)src % 4) == 0) && ((count % 4) == 0))
    {
        uint32_t *destt = (uint32_t *)dest;
        const uint32_t *srcc = (const uint32_t *)src;

        for (uint32_t i = 0; i < (count / 4); ++i)
            destt[i] = srcc[i];
    }
    else if ((((uint32_t)dest % 2) == 0) && (((uint32_t)src % 2) == 0) && ((count % 2) == 0))
    {
        uint16_t *destt = (uint16_t *)dest;
        const uint16_t *srcc = (const uint16_t *)src;

        for (uint32_t i = 0; i < (count / 2); ++i)
            destt[i] = srcc[i];
    }
    else
    {
        uint8_t *destt = (uint8_t *)dest;
        const uint8_t *srcc = (const uint8_t *)src;

        for (uint32_t i = 0; i < count; ++i)
            destt[i] = srcc[i];
    }
}

uint8_t memcmp(const void *p1, const void *p2, uint32_t count)
{
    const uint8_t *pp1 = (const uint8_t *)p1;
    const uint8_t *pp2 = (const uint8_t *)p2;

    for (uint32_t i = 0; i < count; ++i)
    {
        if (pp1[i] != pp2[i])
            return 1;
    }

    return 0;
}

uint8_t memcmp32(const void *p1, const void *p2, uint32_t count)
{
    const uint32_t *pp1 = (const uint32_t *)p1;
    const uint32_t *pp2 = (const uint32_t *)p2;

    for (uint32_t i = 0; i < (count / 4); ++i)
    {
        if (pp1[i] != pp2[i])
            return 1;
    }

    return 0;
}

uint64_t strlen(const char *str)
{
    uint64_t len = 0;

    while (1)
    {
        if (str[len] == 0)
            break;

        ++len;
    }

    return len;
}

uint8_t strcmp(const char *str1, const char *str2)
{
    uint64_t str1_len = strlen(str1);
    uint64_t str2_len = strlen(str2);

    if (str1_len != str2_len)
        return 1;

    for (uint64_t i = 0; i < str2_len; ++i)
    {
        if (str1[i] != str2[i])
            return 1;
    }

    return 0;
}

#endif

struct __attribute__((aligned(8))) DMACmd_s
{
    uint32_t ls;
    uint64_t ea;

    uint16_t size;

    uint16_t cmd;
};

void DMAWait()
{
    uint32_t DMA_TAG_MASK = 1; // (1 << TAG_IDX)

    //
    spu_writech(MFC_WrTagUpdate, 0);
    while (spu_readchcnt(MFC_WrTagUpdate) == 0)
    {
    }
    spu_readch(MFC_RdTagStat);

    //
    spu_writech(MFC_WrTagMask, DMA_TAG_MASK);

    //
    while (spu_mfcstat(MFC_TAG_UPDATE_IMMEDIATE) != DMA_TAG_MASK)
    {
    }
}

void SubmitDMACmd(struct DMACmd_s *cmd)
{
#if 0

    {
        struct DMACmd_s* lastCmd = (struct DMACmd_s*)0x10;

        lastCmd->ls = cmd->ls;
        lastCmd->ea = cmd->ea;

        lastCmd->size = cmd->size;

        lastCmd->cmd = cmd->cmd;
    }

#endif

#if 1

    if ((cmd->ls & 0xf) != (cmd->ea & 0xf))
        stop(2);

    if ((cmd->size > 1) && ((cmd->ea & 0xf) != 0))
        stop(2);

    if (((cmd->size != 1) && (cmd->size != 2) && (cmd->size != 4) && (cmd->size != 8) && ((cmd->size % 16) != 0)) || (cmd->size > (16 * 1024)))
        stop(2);

    if ((cmd->cmd != MFC_PUT_CMD) && (cmd->cmd != MFC_GET_CMD))
        stop(2);

#endif

    spu_writech(MFC_LSA, cmd->ls);

    spu_writech(MFC_EAH, (uint32_t)(cmd->ea >> 32));
    spu_writech(MFC_EAL, (uint32_t)(cmd->ea));

    spu_writech(MFC_Size, cmd->size);

    spu_writech(MFC_TagID, 0);

    // sync();

    spu_writech(MFC_Cmd, (uint32_t)cmd->cmd); // upper 16 bits is classid, must be 0
    // sync();

    // DMAWait();
}

void DMARead(void *ls, uint64_t ea, uint32_t size)
{
    if (size == 0)
        return;

    static const uint32_t maxChunkSize = (16 * 1024);

    uint32_t left = size;

    uint32_t cur_ls = (uint32_t)ls;
    uint64_t cur_ea = ea;

    uint8_t force1 = ((ea & 0xf) != 0);
    uint8_t unalign = ((cur_ls & 0xf) != (ea & 0xf));

    uint8_t *tmpBuf = (uint8_t *)0x3FFF0;

    while (1)
    {
        uint32_t chunkSize = (left > maxChunkSize) ? maxChunkSize : left;

        struct DMACmd_s cmd;
        cmd.cmd = MFC_GET_CMD;

        if (unalign)
        {
            chunkSize = 1;

            uint32_t off = (cur_ea & 0xf);

            cmd.ls = (uint32_t)&tmpBuf[off];
            cmd.ea = cur_ea;

            cmd.size = chunkSize;
            SubmitDMACmd(&cmd);
            DMAWait();

            *((uint8_t *)cur_ls) = tmpBuf[off];
        }
        else
        {
            cmd.ls = cur_ls;
            cmd.ea = cur_ea;

            if (!force1 && (chunkSize >= 16))
                chunkSize -= (chunkSize % 16);
            else
                chunkSize = 1;

            cmd.size = chunkSize;
            SubmitDMACmd(&cmd);
        }

        cur_ls += chunkSize;
        cur_ea += chunkSize;

        left -= chunkSize;

        if (left == 0)
            break;
    }

    if (!unalign)
        DMAWait();
}

void DMAWrite(const void *ls, uint64_t ea, uint32_t size)
{
    if (size == 0)
        return;

    static const uint32_t maxChunkSize = (16 * 1024);

    uint32_t left = size;

    uint32_t cur_ls = (uint32_t)ls;
    uint64_t cur_ea = ea;

    uint8_t force1 = ((ea & 0xf) != 0);
    uint8_t unalign = ((cur_ls & 0xf) != (ea & 0xf));

    uint8_t *tmpBuf = (uint8_t *)0x3FFF0;

    while (1)
    {
        uint32_t chunkSize = (left > maxChunkSize) ? maxChunkSize : left;

        struct DMACmd_s cmd;
        cmd.cmd = MFC_PUT_CMD;

        if (unalign)
        {
            chunkSize = 1;

            uint32_t off = (cur_ea & 0xf);
            tmpBuf[off] = *((const uint8_t *)cur_ls);

            cmd.ls = (uint32_t)&tmpBuf[off];
            cmd.ea = cur_ea;

            cmd.size = chunkSize;
            SubmitDMACmd(&cmd);
            DMAWait();
        }
        else
        {
            cmd.ls = cur_ls;
            cmd.ea = cur_ea;

            if (!force1 && (chunkSize >= 16))
                chunkSize -= (chunkSize % 16);
            else
                chunkSize = 1;

            cmd.size = chunkSize;
            SubmitDMACmd(&cmd);
        }

        cur_ls += chunkSize;
        cur_ea += chunkSize;

        left -= chunkSize;

        if (left == 0)
            break;
    }

    if (!unalign)
        DMAWait();
}