struct CallLv1Function_Context_s
{
	uint64_t num;
	uint64_t args[7];

	uint64_t out[8];
};

extern void CallLv1Function(CallLv1Function_Context_s *ctx);

extern uint64_t lv1_peek_114(uint64_t addr);
extern void lv1_poke_114(uint64_t addr, uint64_t val);

extern void lv1_read_114(uint64_t addr, uint64_t size, void *out_Buf);
extern void lv1_write_114(uint64_t addr, uint64_t size, const void *in_Buf);

extern uint64_t lv1_peek(uint64_t addr);
extern void lv1_poke(uint64_t addr, uint64_t val);

extern uint32_t lv1_peek32(uint64_t addr);
extern void lv1_poke32(uint64_t addr, uint32_t val);

extern void lv1_read(uint64_t addr, uint64_t size, void *out_Buf);
extern void lv1_write(uint64_t addr, uint64_t size, const void *in_Buf);

extern int32_t lv1_allocate_memory(uint64_t size, uint64_t page_size_exp, uint64_t unknown, uint64_t flags, uint64_t *out_lpar_addr, uint64_t *muid);
extern int32_t lv1_release_memory(uint64_t lpar_addr);

extern int32_t lv1_write_htab_entry(uint64_t vas_id, uint64_t slot, uint64_t va, uint64_t pa);

extern int32_t lv1_insert_htab_entry(uint64_t vas_id, uint64_t hpte_group, uint64_t hpte_v, uint64_t hpte_r, uint64_t bolted, uint64_t flags,
									 uint64_t *out_hpte_index, uint64_t *out_hpte_evicted_v, uint64_t *out_hpte_evicted_r);

extern int32_t lv1_map_physical_address_region(uint64_t phys_addr, uint64_t page_size, uint64_t size, uint64_t *out_lpar_addr);
extern int32_t lv1_unmap_physical_address_region(uint64_t lpar_addr);

extern int32_t lv1_construct_virtual_address_space(uint64_t htab_size, uint64_t number_of_sizes, uint64_t page_sizes, uint64_t *vas_id, uint64_t *act_htab_size);
extern int32_t lv1_destruct_virtual_address_space(uint64_t vas_id);

extern int32_t lv1_select_virtual_address_space(uint64_t vas_id);

extern int32_t lv1_map_htab(uint64_t vas_id, uint64_t *htab_addr);
extern int32_t lv1_unmap_htab(uint64_t htab_addr);

extern int32_t lv1_query_logical_partition_address_region_info(uint64_t lpar_addr,
															   uint64_t *start_address, uint64_t *size, uint64_t *access_right, uint64_t *max_page_size, uint64_t *flags);

extern int32_t lv1_get_virtual_address_space_id_of_ppe(uint64_t ppe_id, uint64_t *vas_id);

extern int32_t lv1_get_logical_ppe_id(uint64_t *ppe_id);

extern uint64_t lv1_repository_string(const char *str);

extern int32_t lv1_modify_repository_node_value(uint64_t lpar_id, uint64_t n1, uint64_t n2, uint64_t n3, uint64_t n4, uint64_t v1, uint64_t v2);