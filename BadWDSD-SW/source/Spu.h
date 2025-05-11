extern uint64_t SPU_CalcMMIOAddress(uint64_t spu_id, uint64_t offset);

extern uint64_t SPU_CalcMMIOAddress_LS(uint64_t spu_id, uint64_t offset);

extern uint64_t SPU_CalcMMIOAddress_PS(uint64_t spu_id, uint64_t offset);
extern uint64_t SPU_CalcMMIOAddress_P2(uint64_t spu_id, uint64_t offset);

extern uint64_t SPU_LS_Read64(uint64_t spu_id, uint64_t offset);
extern void SPU_LS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value);

extern uint32_t SPU_LS_Read32(uint64_t spu_id, uint64_t offset);
extern void SPU_LS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value);

extern uint64_t SPU_PS_Read64(uint64_t spu_id, uint64_t offset);
extern void SPU_PS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value);

extern uint32_t SPU_PS_Read32(uint64_t spu_id, uint64_t offset);
extern void SPU_PS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value);

extern uint64_t SPU_P2_Read64(uint64_t spu_id, uint64_t offset);
extern void SPU_P2_Write64(uint64_t spu_id, uint64_t offset, uint64_t value);

extern uint32_t SPU_P2_Read32(uint64_t spu_id, uint64_t offset);
extern void SPU_P2_Write32(uint64_t spu_id, uint64_t offset, uint32_t value);