#include "Include.h"

uint32_t XdrCmd_GetValue(volatile struct XdrCmd_s *cmd)
{
    return cmd->value;
}

void XdrCmd_SetValue(volatile struct XdrCmd_s *cmd, uint32_t value)
{
    cmd->value = value;
}

void XdrCmd_SetStart(volatile struct XdrCmd_s *cmd, uint8_t value)
{
    cmd->value &= ~XdrCmd_Start_Mask;
    cmd->value |= (value << XdrCmd_Start_ShiftCount) & XdrCmd_Start_Mask;
}

uint8_t XdrCmd_GetScmd(volatile struct XdrCmd_s *cmd)
{
    return (cmd->value & XdrCmd_Scmd_Mask) >> XdrCmd_Scmd_ShiftCount;
}

void XdrCmd_SetScmd(volatile struct XdrCmd_s *cmd, uint8_t value)
{
    cmd->value &= ~XdrCmd_Scmd_Mask;
    cmd->value |= ((uint32_t)value << XdrCmd_Scmd_ShiftCount) & XdrCmd_Scmd_Mask;
}

void XdrCmd_SetSid(volatile struct XdrCmd_s *cmd, uint8_t value)
{
    cmd->value &= ~XdrCmd_Sid_Mask;
    cmd->value |= ((uint32_t)value << XdrCmd_Sid_ShiftCount) & XdrCmd_Sid_Mask;
}

void XdrCmd_SetSadr(volatile struct XdrCmd_s *cmd, uint8_t value)
{
    cmd->value &= ~XdrCmd_Sadr_Mask;
    cmd->value |= ((uint32_t)value << XdrCmd_Sadr_ShiftCount) & XdrCmd_Sadr_Mask;
}

void XdrCmd_SetJunk1(volatile struct XdrCmd_s *cmd, uint8_t value)
{
    cmd->value &= ~XdrCmd_Junk1_Mask;
    cmd->value |= ((uint32_t)value << XdrCmd_Junk1_ShiftCount) & XdrCmd_Junk1_Mask;
}

void XdrCmd_SetSwd(volatile struct XdrCmd_s *cmd, uint8_t value)
{
    cmd->value &= ~XdrCmd_Swd_Mask;
    cmd->value |= ((uint32_t)value << XdrCmd_Swd_ShiftCount) & XdrCmd_Swd_Mask;
}

void XdrCmd_SetJunk2(volatile struct XdrCmd_s *cmd, uint8_t value)
{
    cmd->value &= ~XdrCmd_Junk2_Mask;
    cmd->value |= ((uint32_t)value << XdrCmd_Junk2_ShiftCount) & XdrCmd_Junk2_Mask;
}

void Xdr_SendCmd(volatile struct XdrCmd_s cmd)
{
    XdrCmd_SetStart(&cmd, 0xc);

    XdrCmd_SetJunk1(&cmd, 0);
    XdrCmd_SetJunk2(&cmd, 0);

    uint8_t scmd = XdrCmd_GetScmd(&cmd);

    if (scmd == 0x2 || scmd == 0x3) // read
        XdrCmd_SetSwd(&cmd, 0);

    uint32_t value = XdrCmd_GetValue(&cmd);

    //

    uint32_t i = 31;

    while (1)
    {
        //

        GPIO_FLOAT2(XDR_GPO_CLK_PIN_ID, XDR_GPO_CLK_PIN_ID2);
        XDR_GPO_DELAY();

        //

        bool b = ((value & (1 << i)) != 0);

        if (b)
            GPIO_FLOATTOLOW2(XDR_GPO_CMD_PIN_ID, XDR_GPO_CMD_PIN_ID2);
        else
            GPIO_FLOAT2(XDR_GPO_CMD_PIN_ID, XDR_GPO_CMD_PIN_ID2);

        XDR_GPO_DELAY();

        //

        GPIO_FLOATTOLOW2(XDR_GPO_CLK_PIN_ID, XDR_GPO_CLK_PIN_ID2);
        XDR_GPO_DELAY();
        XDR_GPO_DELAY();

        //

        if (i == 0)
            break;

        --i;
    }

    //

    GPIO_FLOAT2(XDR_GPO_CLK_PIN_ID, XDR_GPO_CLK_PIN_ID2);
    XDR_GPO_DELAY();

    //

    GPIO_FLOAT2(XDR_GPO_CMD_PIN_ID, XDR_GPO_CMD_PIN_ID2);
}

void Xdr_SendReadROM0()
{
    struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x3); // forced read
    XdrCmd_SetSid(&cmd, 0x0);

    XdrCmd_SetSadr(&cmd, 0x16); // ROM0

    Xdr_SendCmd(cmd);
}

void Xdr_SendEnableSLE_x16()
{
    struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x1); // broadcast write
    XdrCmd_SetSid(&cmd, 0x0);

    XdrCmd_SetSadr(&cmd, 0x2); // CFG
    XdrCmd_SetSwd(&cmd, 0x14); // SLE Enabled, x16

    Xdr_SendCmd(cmd);
}

void Xdr_SendDisableSLE_x16()
{
    struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x1); // broadcast write
    XdrCmd_SetSid(&cmd, 0x0);

    XdrCmd_SetSadr(&cmd, 0x2); // CFG
    XdrCmd_SetSwd(&cmd, 0x4);  // SLE Disabled, x16

    Xdr_SendCmd(cmd);
}

void Xdr_SendEnableSLE_x16_PerDevice(uint8_t sid)
{
    struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x0); // device write
    XdrCmd_SetSid(&cmd, sid);

    XdrCmd_SetSadr(&cmd, 0x2); // CFG
    XdrCmd_SetSwd(&cmd, 0x14); // SLE Enabled, x16

    Xdr_SendCmd(cmd);
}

void Xdr_SendDisableSLE_x16_PerDevice(uint8_t sid)
{
    struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x0); // device write
    XdrCmd_SetSid(&cmd, sid);

    XdrCmd_SetSadr(&cmd, 0x2); // CFG
    XdrCmd_SetSwd(&cmd, 0x4);  // SLE Disabled, x16

    Xdr_SendCmd(cmd);
}

void Xdr_SendWDSD_x16(const uint8_t *wdslData)
{
    volatile struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x1); // broadcast write
    XdrCmd_SetSid(&cmd, 0x0);

    XdrCmd_SetSadr(&cmd, 0x4); // WDSD

    for (uint32_t i = 0; i < 32; ++i)
    {
        XdrCmd_SetSwd(&cmd, wdslData[i]);
        Xdr_SendCmd(cmd);
    }
}

void Xdr_SendWDSD_x16_PerDevice(uint8_t sid, const uint8_t *wdslData)
{
    volatile struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x0); // device write
    XdrCmd_SetSid(&cmd, sid);

    XdrCmd_SetSadr(&cmd, 0x4); // WDSD

    for (uint32_t i = 0; i < 32; ++i)
    {
        XdrCmd_SetSwd(&cmd, wdslData[i]);
        Xdr_SendCmd(cmd);
    }
}

void Xdr_SendEnableSLE_x32()
{
    struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x1); // broadcast write
    XdrCmd_SetSid(&cmd, 0x0);

    XdrCmd_SetSadr(&cmd, 0x2); // CFG
    XdrCmd_SetSwd(&cmd, 0x15); // SLE Enabled, x32

    Xdr_SendCmd(cmd);
}

void Xdr_SendDisableSLE_x32()
{
    struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x1); // broadcast write
    XdrCmd_SetSid(&cmd, 0x0);

    XdrCmd_SetSadr(&cmd, 0x2); // CFG
    XdrCmd_SetSwd(&cmd, 0x5);  // SLE Disabled, x32

    Xdr_SendCmd(cmd);
}

void Xdr_SendEnableSLE_x32_PerDevice(uint8_t sid)
{
    struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x0); // device write
    XdrCmd_SetSid(&cmd, sid);

    XdrCmd_SetSadr(&cmd, 0x2); // CFG
    XdrCmd_SetSwd(&cmd, 0x15); // SLE Enabled, x32

    Xdr_SendCmd(cmd);
}

void Xdr_SendDisableSLE_x32_PerDevice(uint8_t sid)
{
    struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x0); // device write
    XdrCmd_SetSid(&cmd, sid);

    XdrCmd_SetSadr(&cmd, 0x2); // CFG
    XdrCmd_SetSwd(&cmd, 0x5);  // SLE Disabled, x32

    Xdr_SendCmd(cmd);
}

void Xdr_SendWDSD_x32(const uint8_t *wdslData)
{
    volatile struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x1); // broadcast write
    XdrCmd_SetSid(&cmd, 0x0);

    XdrCmd_SetSadr(&cmd, 0x4); // WDSD

    for (uint32_t i = 0; i < 64; ++i)
    {
        XdrCmd_SetSwd(&cmd, wdslData[i]);
        Xdr_SendCmd(cmd);
    }
}

void Xdr_SendWDSD_x32_PerDevice(uint8_t sid, const uint8_t *wdslData)
{
    volatile struct XdrCmd_s cmd;

    XdrCmd_SetScmd(&cmd, 0x0); // device write
    XdrCmd_SetSid(&cmd, sid);

    XdrCmd_SetSadr(&cmd, 0x4); // WDSD

    for (uint32_t i = 0; i < 64; ++i)
    {
        XdrCmd_SetSwd(&cmd, wdslData[i]);
        Xdr_SendCmd(cmd);
    }
}

uint16_t Xdr_ConvertToWDSLWord(uint16_t inData)
{
    uint16_t result;
    uint8_t bit_order[16] =
        {15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0};

    result = 0;
    for (uint32_t n = 0; n < 16; ++n)
    {
        if ((inData >> bit_order[n]) & 0x0001)
        {
            result |= (0x8000 >> n);
        }
    }

    return result;
}

// [32]
void Xdr_ConvertDataToWDSLData_x16(const uint8_t *inData, uint8_t *outWDSLData)
{
    const uint16_t *data = (const uint16_t *)inData;
    uint16_t *wdslData = (uint16_t *)outWDSLData;

    for (uint32_t i = 0; i < 16; ++i)
    {
        wdslData[i] = Xdr_ConvertToWDSLWord(data[i]);
        wdslData[i] = swap_uint16(wdslData[i]);
    }
}

// [64]
void Xdr_ConvertDataToWDSLData_x32(const uint8_t *inData, uint8_t *outWDSLData)
{
    const uint16_t *data = (const uint16_t *)inData;
    uint16_t *wdslData = (uint16_t *)outWDSLData;

    for (uint32_t i = 0; i < 32; ++i)
    {
        wdslData[i] = Xdr_ConvertToWDSLWord(data[i]);
        wdslData[i] = swap_uint16(wdslData[i]);
    }
}

// inData[32], outWDSLData0[32], outWDSLData1[32]
void Xdr_GenerateReadyWDSLData_x16(const uint8_t *inData, uint8_t *outWDSLData0, uint8_t *outWDSLData1)
{
    uint8_t tmpData0[32];
    uint8_t tmpData1[32];

    uint8_t map[64];

    {
        uint64_t *m = (uint64_t *)map;

        m[0] = swap_uint64(0x2b252d211b1d4d27);
        m[1] = swap_uint64(0x29472341174b112f);
        m[2] = swap_uint64(0x453f43353931334f);
        m[3] = swap_uint64(0x1f194913373d3b15);

        m[4] = swap_uint64(0x2a242c201a1c4c26);
        m[5] = swap_uint64(0x28462240164a102e);
        m[6] = swap_uint64(0x443e42343830324e);
        m[7] = swap_uint64(0x1e184812363c3a14);
    }

    for (uint32_t i = 0; i < 64; ++i)
        map[i] -= 0x10;

    for (uint32_t i = 0; i < 64; ++i)
    {
        uint8_t o = map[i];

        if (o >= 32)
            tmpData1[(o % 32)] = inData[(i % 32)];
        else
            tmpData0[(o % 32)] = inData[(i % 32)];
    }

    Xdr_ConvertDataToWDSLData_x16(tmpData0, outWDSLData0);
    Xdr_ConvertDataToWDSLData_x16(tmpData1, outWDSLData1);
}

// inData[64], outWDSLData0[64]
void Xdr_GenerateReadyWDSLData_x32(const uint8_t* inData, uint8_t* outWDSLData0)
{
    uint8_t tmpData0[64];

    uint8_t map[64];

    {
        uint64_t *m = (uint64_t *)map;

        m[0] = swap_uint64(0x343a384a48464436);
        m[1] = swap_uint64(0x40324c303c3e4e42);
        m[2] = swap_uint64(0x162e1e2c22182012);
        m[3] = swap_uint64(0x2a281024141a1c26);

        m[4] = swap_uint64(0x353b394b49474537);
        m[5] = swap_uint64(0x41334d313d3f4f43);
        m[6] = swap_uint64(0x172f1f2d23192113);
        m[7] = swap_uint64(0x2b291125151b1d27);
    }

    for (uint32_t i = 0; i < 64; ++i)
        map[i] -= 0x10;

    for (uint32_t i = 0; i < 64; ++i)
    {
        uint8_t o = map[i];

        tmpData0[o] = inData[i];
    }

    Xdr_ConvertDataToWDSLData_x32(tmpData0, outWDSLData0);
}