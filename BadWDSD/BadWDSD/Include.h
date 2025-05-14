//

//#define PICO_IS_ZERO 1

#define SC_IS_SW 1
#define XDR_IS_X32 1

//

#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include "pico/stdlib.h"

#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"

#include "pico/multicore.h"
#include "pico/rand.h"

#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"

#define sync() __dsb();

extern void dead();

extern uint64_t get_time_in_us();
extern uint64_t get_time_in_ms();

extern void WaitInNs(uint64_t ns);

static const uint32_t HOLD_PIN_ID = 2;

extern void Hold_Init();

#if PICO_IS_ZERO
static const uint32_t LED_PIN_ID = 11;
#else
static const uint32_t LED_PIN_ID = PICO_DEFAULT_LED_PIN;
#endif

static const uint32_t LED_STATUS_OFF = 0;
static const uint32_t LED_STATUS_ON = 1;

static const uint32_t LED_STATUS_BLINK = 2;

struct LedContext_s
{
    uint32_t status; // 0 = off, 1 = on, 2 = blink
    uint32_t blinkIntervalInMs;

    uint32_t prevStatus;

    bool curLedStatus;

    uint64_t blink_t1;
};

extern volatile bool ledIsInited;
extern volatile struct LedContext_s ledContext;

extern void Led_SetStatus(uint32_t status);
extern void Led_SetBlinkIntervalInMs(uint32_t value);

extern void Led_Init();

// second core
extern void Led_Thread();

//
extern bool Led_IsInited();

// XDRs

#if PICO_IS_ZERO

static const uint32_t XDR_GPO_CLK_PIN_ID = 6;
static const uint32_t XDR_GPO_CLK_PIN_ID2 = 7;

static const uint32_t XDR_GPO_CMD_PIN_ID = 10;
static const uint32_t XDR_GPO_CMD_PIN_ID2 = 9;

#else

static const uint32_t XDR_GPO_CLK_PIN_ID = 6;
static const uint32_t XDR_GPO_CLK_PIN_ID2 = 7;

static const uint32_t XDR_GPO_CMD_PIN_ID = 10;
static const uint32_t XDR_GPO_CMD_PIN_ID2 = 11;

#endif

#if 0

#define XDR_GPO_DELAY_ENABLED 1
#define XDR_GPO_DELAY_VALUE_IN_NS 300 // 1000ns/1mhz per cycle

#if XDR_GPO_DELAY_ENABLED
#define XDR_GPO_DELAY() WaitInNs(XDR_GPO_DELAY_VALUE_IN_NS)
#else
#define XDR_GPO_DELAY()
#endif

#else

#define XDR_GPO_DELAY_ENABLED 1
//#define XDR_GPO_DELAY_VALUE_IN_US 1 // 4000ns/250khz per cycle
//#define XDR_GPO_DELAY_VALUE_IN_US 2 // 8000ns/125khz per cycle
#define XDR_GPO_DELAY_VALUE_IN_US 4 // 16000ns/62.5khz per cycle
//#define XDR_GPO_DELAY_VALUE_IN_US 8 // 32000ns/31.25khz per cycle

#if XDR_GPO_DELAY_ENABLED
#define XDR_GPO_DELAY() busy_wait_us(XDR_GPO_DELAY_VALUE_IN_US)
#else
#define XDR_GPO_DELAY()
#endif

#endif

#if 0

union XdrCmd_u
{
    struct
    {
        // always 0xc
        uint8_t start : 4;

        // 0x0 = Serial device write
        // 0x1 = Serial broadcast write
        // 0x2 = Serial device read
        // 0x3 = Serial forced read
        uint8_t scmd : 2;

        // bit 6 and 7 must be 0
        uint8_t sid;

        //
        uint8_t sadr;

        // must be 0
        bool junk1 : 1;

        // must be 0 if read
        uint8_t swd;

        // must be 0
        bool junk2 : 1;
    };

    uint32_t value;
};

#endif

static const uint32_t XdrCmd_Start_Mask = 0xF0000000;
static const uint32_t XdrCmd_Start_ShiftCount = 28;

static const uint32_t XdrCmd_Scmd_Mask = 0xC000000;
static const uint32_t XdrCmd_Scmd_ShiftCount = 26;

static const uint32_t XdrCmd_Sid_Mask = 0x3FC0000;
static const uint32_t XdrCmd_Sid_ShiftCount = 18;

static const uint32_t XdrCmd_Sadr_Mask = 0x3FC00;
static const uint32_t XdrCmd_Sadr_ShiftCount = 10;

static const uint32_t XdrCmd_Junk1_Mask = 0x200;
static const uint32_t XdrCmd_Junk1_ShiftCount = 9;

static const uint32_t XdrCmd_Swd_Mask = 0x1FE;
static const uint32_t XdrCmd_Swd_ShiftCount = 1;

static const uint32_t XdrCmd_Junk2_Mask = 0x1;
static const uint32_t XdrCmd_Junk2_ShiftCount = 0;

struct XdrCmd_s
{
    uint32_t value;
};

extern uint32_t XdrCmd_GetValue(volatile struct XdrCmd_s* cmd);
extern void XdrCmd_SetValue(volatile struct XdrCmd_s* cmd, uint32_t value);

extern void XdrCmd_SetStart(volatile struct XdrCmd_s* cmd, uint8_t value);

extern uint8_t XdrCmd_GetScmd(volatile struct XdrCmd_s* cmd);
extern void XdrCmd_SetScmd(volatile struct XdrCmd_s* cmd, uint8_t value);

extern void XdrCmd_SetSid(volatile struct XdrCmd_s* cmd, uint8_t value);

extern void XdrCmd_SetSadr(volatile struct XdrCmd_s* cmd, uint8_t value);

extern void XdrCmd_SetJunk1(volatile struct XdrCmd_s* cmd, uint8_t value);

extern void XdrCmd_SetSwd(volatile struct XdrCmd_s* cmd, uint8_t value);

extern void XdrCmd_SetJunk2(volatile struct XdrCmd_s* cmd, uint8_t value);

extern void Xdr_SendCmd(volatile struct XdrCmd_s cmd);

extern void Xdr_SendReadROM0();

//

extern void Xdr_SendEnableSLE_x16();
extern void Xdr_SendDisableSLE_x16();

extern void Xdr_SendEnableSLE_x16_PerDevice(uint8_t sid);
extern void Xdr_SendDisableSLE_x16_PerDevice(uint8_t sid);

// data[32]
extern void Xdr_SendWDSD_x16(const uint8_t* wdslData);
extern void Xdr_SendWDSD_x16_PerDevice(uint8_t sid, const uint8_t* wdslData);

//

extern void Xdr_SendEnableSLE_x32();
extern void Xdr_SendDisableSLE_x32();

extern void Xdr_SendEnableSLE_x32_PerDevice(uint8_t sid);
extern void Xdr_SendDisableSLE_x32_PerDevice(uint8_t sid);

// data[64]
extern void Xdr_SendWDSD_x32(const uint8_t* wdslData);
extern void Xdr_SendWDSD_x32_PerDevice(uint8_t sid, const uint8_t* wdslData);

//

extern uint16_t Xdr_ConvertToWDSLWord(uint16_t inData);

extern void Xdr_ConvertDataToWDSLData_x16(const uint8_t* inData, uint8_t* outWDSLData);
extern void Xdr_ConvertDataToWDSLData_x32(const uint8_t* inData, uint8_t* outWDSLData);

// inData[32], outWDSLData0[32], outWDSLData1[32]
extern void Xdr_GenerateReadyWDSLData_x16(const uint8_t* inData, uint8_t* outWDSLData0, uint8_t* outWDSLData1);

// inData[32], outWDSLData0[64]
extern void Xdr_GenerateReadyWDSLData_x32(const uint8_t* inData, uint8_t* outWDSLData0);

// Button

//#define BUTTON_MODE_ENABLED 1

#if PICO_IS_ZERO
static const uint32_t BUTTON_PIN_ID = 26;
#else
static const uint32_t BUTTON_PIN_ID = 15;
#endif

extern void Button_Thread();

//

extern void GPIO_FLOATTOLOW(uint32_t pinId);
extern void GPIO_FLOAT(uint32_t pinId);

extern void GPIO_FLOATTOLOW2(uint32_t pinId1, uint32_t pinId2);
extern void GPIO_FLOAT2(uint32_t pinId1, uint32_t pinId2);

extern bool Gpio_GetOnce(uint32_t pinId);

//

extern uint16_t swap_uint16(uint16_t val);
extern uint64_t swap_uint64(uint64_t val);

//

extern void Uart_Init(uart_inst_t* uartId, uint32_t baud, bool rxEnabled, uint32_t rxPinId, bool txEnabled, uint32_t txPinId, void* rxFn);

extern void Uart_Putc(uart_inst_t* uartId, char c);
extern void Uart_Puts(uart_inst_t* uartId, const char* buf);

//

#if PICO_IS_ZERO
static const uint32_t SC_UART_RX_PIN_ID = 13;
static const uint32_t SC_UART_TX_PIN_ID = 12;
#else
static const uint32_t SC_UART_RX_PIN_ID = 17;
static const uint32_t SC_UART_TX_PIN_ID = 16;
#endif

#if SC_IS_SW
static const uint32_t SC_UART_BAUD = 57600;
#else
static const uint32_t SC_UART_BAUD = 115200;
#endif

struct Sc_SendCommandContext_s;

struct ScContext_s
{
    volatile uart_inst_t* uartId;

    volatile char rxBuf[1024];
    volatile uint32_t rxBufLen;

    volatile char txBuf[2048];
    volatile uint32_t txBufLen;

    volatile bool trigger;

    volatile bool success;

    volatile struct Sc_SendCommandContext_s* sendCommandCtx;
};

extern volatile bool scIsInited;
extern volatile struct ScContext_s scContext;

extern void Sc_Thread();

extern bool Sc_IsInited();

#if PICO_IS_ZERO
static const uint32_t SC_BYPASS_PIN_ID = 3;
static const uint32_t SC_BANKSEL_PIN_ID = 14;
static const uint32_t SC_RECOVERY_PIN_ID = 15;
#else
static const uint32_t SC_BYPASS_PIN_ID = 14;
static const uint32_t SC_BANKSEL_PIN_ID = 28;
static const uint32_t SC_RECOVERY_PIN_ID = 22;
#endif

extern bool Sc_GetScBypass();
extern bool Sc_GetScBanksel();
extern bool Sc_GetScRecovery();

extern void Sc_Init();

extern bool Sc_GetTrigger();
extern void Sc_ClearTrigger();

extern bool Sc_GetSuccess();
extern void Sc_ClearSuccess();

extern void Sc_Putc(char c);
extern void Sc_Puts(const char* buf);

struct Sc_SendCommandContext_s
{
    volatile char cmd[1024];

    volatile char expectedResponse[2048];

    volatile char response[2048];
    volatile uint32_t responseLen;

    volatile bool done;
};

extern void Sc_SendCommand(volatile struct Sc_SendCommandContext_s* ctx);

//

#define DEBUG_UART_ENABLED 1

static const uint32_t DEBUG_UART_RX_PIN_ID = 5;
static const uint32_t DEBUG_UART_TX_PIN_ID = 4;

static const uint32_t DEBUG_UART_BAUD = 576000;

struct DebugUartContext_s
{
    uart_inst_t* uartId;

    char txBuf[2048];
    uint32_t txBufLen;
};

extern volatile bool debugUartIsInited;
extern volatile struct DebugUartContext_s debugUartContext;

extern void DebugUart_Thread();

extern bool DebugUart_IsInited();

extern void DebugUart_Init();

extern void DebugUart_Putc(char c);
extern void DebugUart_Puts(const char* buf);

#define PrintLog(...) { char* buf = (char*)malloc(2048); sprintf(buf, __VA_ARGS__); if (DebugUart_IsInited()) DebugUart_Puts(buf); free(buf); }

// aes

#include "Aes.h"

//