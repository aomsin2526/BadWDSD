//

#define PICO_TYPE_E_PICO 1
#define PICO_TYPE_E_PICO_W 2
#define PICO_TYPE_E_RP2040_ZERO 3

// generated at build
#include "build/Config.h"

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
#include "pico/mutex.h"

#if PICO_TYPE == PICO_TYPE_E_PICO_W
#include "pico/cyw43_arch.h"
#endif

#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"

#define sync() __dsb()

extern void dead();

extern uint64_t get_time_in_us();
extern uint64_t get_time_in_ms();

extern void WaitInNs(uint64_t ns);

static const uint32_t HOLD_PIN_ID = 2;

extern void Hold_Init();

#if PICO_TYPE == PICO_TYPE_E_RP2040_ZERO
#define LED_IS_WS2812 1
#define LED_WS2812_PIO pio0
static const uint32_t LED_PIN_ID = 16;
static const uint8_t LED_RGB[3] = {0, 0, 30};
#elif PICO_TYPE == PICO_TYPE_E_PICO
static const uint32_t LED_PIN_ID = PICO_DEFAULT_LED_PIN;
#elif PICO_TYPE == PICO_TYPE_E_PICO_W
#define LED_IS_NOT_GPIO 1
#else
#error bad!!!
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

extern void Led_SetStatus(uint32_t status);
extern void Led_SetBlinkIntervalInMs(uint32_t value);

extern void Led_Init();

// second core
extern void Led_Thread();

//
extern bool Led_IsInited();

// XDRs

static const uint32_t XDR_GPO_CLK_PIN_ID = 6;
static const uint32_t XDR_GPO_CMD_PIN_ID = 10;

#if IS_EMMC
static const uint32_t XDR_GPO_CLK_PIN_ID2 = XDR_GPO_CLK_PIN_ID;
static const uint32_t XDR_GPO_CMD_PIN_ID2 = XDR_GPO_CMD_PIN_ID;
#else
static const uint32_t XDR_GPO_CLK_PIN_ID2 = 7;
static const uint32_t XDR_GPO_CMD_PIN_ID2 = 11;
#endif

#if !IS_EMMC
#define XDR_GPO_DELAY_ENABLED 1
#endif

#define XDR_GPO_DELAY_VALUE_IN_US 1 // 4000ns/250khz per cycle
//#define XDR_GPO_DELAY_VALUE_IN_US 2 // 8000ns/125khz per cycle
//#define XDR_GPO_DELAY_VALUE_IN_US 4 // 16000ns/62.5khz per cycle
//#define XDR_GPO_DELAY_VALUE_IN_US 8 // 32000ns/31.25khz per cycle

#if XDR_GPO_DELAY_ENABLED
#define XDR_GPO_DELAY() busy_wait_us(XDR_GPO_DELAY_VALUE_IN_US)
#else
#define XDR_GPO_DELAY()
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

extern uint32_t XdrCmd_GetValue(struct XdrCmd_s* cmd);
extern void XdrCmd_SetValue(struct XdrCmd_s* cmd, uint32_t value);

extern void XdrCmd_SetStart(struct XdrCmd_s* cmd, uint8_t value);

extern uint8_t XdrCmd_GetScmd(struct XdrCmd_s* cmd);
extern void XdrCmd_SetScmd(struct XdrCmd_s* cmd, uint8_t value);

extern void XdrCmd_SetSid(struct XdrCmd_s* cmd, uint8_t value);

extern void XdrCmd_SetSadr(struct XdrCmd_s* cmd, uint8_t value);

extern void XdrCmd_SetJunk1(struct XdrCmd_s* cmd, uint8_t value);

extern void XdrCmd_SetSwd(struct XdrCmd_s* cmd, uint8_t value);

extern void XdrCmd_SetJunk2(struct XdrCmd_s* cmd, uint8_t value);

extern uint32_t XdrCmd_GetValueForSendRaw(struct XdrCmd_s cmd);

extern void Xdr_SendRawCmd(uint32_t value);

extern void Xdr_SendCmd(struct XdrCmd_s cmd);

//

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

extern uint32_t Xdr_PrepareSendEnableSLE_x32_PerDeviceRaw(uint8_t sid);
extern void Xdr_SendEnableSLE_x32_PerDevice(uint8_t sid);

extern void Xdr_SendDisableSLE_x32_PerDevice(uint8_t sid);

// data[64]
extern void Xdr_SendWDSD_x32(const uint8_t* wdslData);

extern void Xdr_PrepareSendWDSD_x32_PerDeviceRaw(uint8_t sid, const uint8_t *wdslData, uint32_t* outRawValues);
extern void Xdr_SendWDSD_x32_PerDevice(uint8_t sid, const uint8_t* wdslData);

//

extern uint16_t Xdr_ConvertToWDSLWord(uint16_t inData);

extern void Xdr_ConvertDataToWDSLData_x16(const uint8_t* inData, uint8_t* outWDSLData);
extern void Xdr_ConvertDataToWDSLData_x32(const uint8_t* inData, uint8_t* outWDSLData);

// inData[32], outWDSLData0[32], outWDSLData1[32]
extern void Xdr_GenerateReadyWDSLData_x16(const uint8_t* inData, uint8_t* outWDSLData0, uint8_t* outWDSLData1);

// inData[64], outWDSLData0[64]
extern void Xdr_GenerateReadyWDSLData_x32(const uint8_t* inData, uint8_t* outWDSLData0);

//

extern void GPIO_FLOATTOLOW(uint32_t pinId);
extern void GPIO_FLOATTOHIGH(uint32_t pinId);

extern void GPIO_FLOAT(uint32_t pinId);

extern void GPIO_FLOATTOLOW2(uint32_t pinId1, uint32_t pinId2);
extern void GPIO_FLOAT2(uint32_t pinId1, uint32_t pinId2);

extern bool Gpio_GetOnce(uint32_t pinId);

//

extern uint16_t swap_uint16(uint16_t val);
extern uint64_t swap_uint64(uint64_t val);

//

extern void Uart_Init(uart_inst_t* uartId, uint32_t baud, bool rxEnabled, uint32_t rxPinId, bool txEnabled, uint32_t txPinId);
extern void Uart_Uninit(uart_inst_t* uartId, bool rxEnabled, uint32_t rxPinId, bool txEnabled, uint32_t txPinId);

extern void Uart_Putc(uart_inst_t* uartId, char c);
extern void Uart_Puts(uart_inst_t* uartId, const char* buf);

//

#if PICO_TYPE == PICO_TYPE_E_RP2040_ZERO
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

#define SC_RXBUF_SIZE 1024
#define SC_TXBUF_SIZE 1024

struct Sc_SendCommandContext_s;

struct ScContext_s
{
    uart_inst_t* uartId;

    char rxBuf[SC_RXBUF_SIZE];
    uint32_t rxBufCurLen;

#if IS_EMMC
    uint32_t emmcTriggerCounter;
#endif

    bool trigger;
    bool xdrInitFail;

    bool success;
    bool shutdownSuccess;
    bool bringupSuccess;

    bool needReboot;

    struct Sc_SendCommandContext_s* sendCommandCtx;

    uint64_t lastScTxTimeInMs;
};

extern void Sc_Thread();

extern bool Sc_IsInited();
extern bool Sc_IsReadyForUser();

#if PICO_TYPE == PICO_TYPE_E_RP2040_ZERO
static const uint32_t SC_LITE_PIN_ID = 3;
static const uint32_t SC_BANKSEL_PIN_ID = 14;
#else
static const uint32_t SC_LITE_PIN_ID = 14;
static const uint32_t SC_BANKSEL_PIN_ID = 28;
#endif

extern bool Sc_GetScLite();
extern bool Sc_GetScBanksel();

extern void Sc_Init();

extern bool Sc_GetTrigger();
extern void Sc_ClearTrigger();

extern bool Sc_GetXdrInitFail();
extern void Sc_ClearXdrInitFail();

extern bool Sc_GetSuccess();
extern void Sc_ClearSuccess();

extern bool Sc_GetShutdownSuccess();
extern void Sc_ClearShutdownSuccess();

extern bool Sc_GetBringupSuccess();
extern void Sc_ClearBringupSuccess();

extern bool Sc_GetNeedReboot();
extern void Sc_ClearNeedReboot();

extern void Sc_Puts(const char* cmd);

struct Sc_SendCommandContext_s
{
    char cmd[SC_TXBUF_SIZE];

    char expectedResponse[SC_RXBUF_SIZE];

    char response[SC_RXBUF_SIZE];
    uint32_t responseLen;

    bool done;
};

extern void Sc_SendCommand(struct Sc_SendCommandContext_s* ctx);

//

#define DEBUG_UART_ENABLED 1

static const uint32_t DEBUG_UART_RX_PIN_ID = 5;
static const uint32_t DEBUG_UART_TX_PIN_ID = 4;

static const uint32_t DEBUG_UART_BAUD = 576000;

#define DEBUG_UART_TXBUF_SIZE SC_TXBUF_SIZE

struct DebugUartContext_s
{
    uart_inst_t* uartId;

    char txBuf[DEBUG_UART_TXBUF_SIZE];
    uint32_t txBufCurLen;

    uint64_t lastRxTimeInMs;
};

extern void DebugUart_Thread();

extern bool DebugUart_IsInited();

extern void DebugUart_Init();
extern void DebugUart_Uninit();

extern void DebugUart_Putc(char c);
extern void DebugUart_Puts(const char* buf);

#define PrintLog(...) { if (DebugUart_IsInited()) { char* ___buf = (char*)malloc(16384); if (___buf == NULL) { dead(); } sprintf(___buf, __VA_ARGS__); DebugUart_Puts(___buf); free(___buf); } }

//

static const uint32_t SB_UART_TX_PIN_ID = 8;

static const uint32_t SB_UART_BAUD = 115200;

struct SbContext_s
{
    uart_inst_t* uartId;
};

extern bool Sb_IsInited();

extern void Sb_Init();
extern void Sb_Uninit();

extern void Sb_Putc(char c);

// aes

#include "Aes.h"

//

// kafuu's personal pin for hooking sb tristate (hw flasher) pin to it for convenience

#if PICO_TYPE == PICO_TYPE_E_PICO
static const uint32_t TRISTATE_PIN_ID = 21;
#endif

//