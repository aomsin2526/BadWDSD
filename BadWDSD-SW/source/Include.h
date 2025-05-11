#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

#include <ppu-types.h>

#include <ppu-lv2.h>
#include <sys/systime.h>

#include <sys/thread.h>
#include <sys/file.h>

extern "C"
{
#include <usb/usb.h>
};

#include <sysmodule/sysmodule.h>

#include "Logging.h"
#include "Utils.h"

#include "Lv2.h"
#include "Lv1.h"

#include "Glitcher.h"

#include "LoadLv2Kernel.h"

#include "Lv1Exec.h"

#include "Xorhack.h"

#include "mm.h"

#include "Spu.h"

extern uint64_t CalcHTAB_EA_Addr_By_HtabIdx(uint64_t base, uint32_t htab_idx);
extern uint64_t CalcGameOSHTAB_EA_Addr_By_HtabIdx(uint32_t htab_idx);

extern uint32_t FindFreeHTABIdx();

extern "C"
{
	void testxxx(void);
};

#define eieio()                \
	{                          \
		asm volatile("eieio"); \
		asm volatile("sync");  \
	}
#define isync() asm volatile("isync")

extern double fwVersion;