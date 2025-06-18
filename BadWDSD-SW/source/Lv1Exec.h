extern uint64_t FindHvcallTable();

extern bool IsOurHvcallInstalled();

extern void InstallOurHvcall();

struct CallLv1Exec_Context_s
{
public:
	uint64_t ra;
	uint64_t args[7];

	uint64_t out[8];
};

extern void CallLv1Exec(CallLv1Exec_Context_s* ctx);

struct CallLv1ExecEa_Context_s
{
public:
	uint64_t ea;
	uint64_t size; // size of function

	uint64_t args[7];

	uint64_t out[8];
};

extern void CallLv1ExecEa(CallLv1ExecEa_Context_s* ctx);

extern "C"
{
	extern void our_lv1_exec_do();
	extern uint64_t our_lv1_exec_do_size;

	extern void our_lv1_exec_test_do();
	extern uint64_t our_lv1_exec_test_do_size;

	extern void our_lv1_test_puts_do();
	extern uint64_t our_lv1_test_puts_do_size;

	extern void our_lv1_auth_lv2_hook_fself_do();
	extern uint64_t our_lv1_auth_lv2_hook_fself_do_size;

	extern void our_lv1_apply_rsx_clock_do();
	extern uint64_t our_lv1_apply_rsx_clock_do_size;

	extern uint8_t __start_BadWDSD_Stage1_Test_Do_Section[];
	extern uint8_t __stop_BadWDSD_Stage1_Test_Do_Section[];
};

extern void lv1_test_puts();

extern void lv1_apply_rsx_clock(uint64_t core_mul, uint64_t mem_mul);

extern void BadWDSD_Stage1_Test();

extern void BadWDSD_Stage1_Bin_Test();

extern void BadWDSD_Stage1_Bin_Flash_Test(bool exec);