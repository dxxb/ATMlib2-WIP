
#include "atm_synth_internal.h"
#include "atm_pool.h"

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

void atm_pool_init(void)
{
	for (int i = 0; i < atm_mem_slot_count; i++) {
		atm_mem_pool[i].next = i+1;
	}
	atm_mem_pool[atm_mem_slot_count-1].next = ATM_POOL_INVALID_SLOT;
	atm_pool_free_list_head = 0;
}

atm_pool_slot_idx_t atm_pool_alloc1(atm_pool_slot_idx_t next)
{
	const atm_pool_slot_idx_t next_free = atm_pool_free_list_head;
	if (next_free != ATM_POOL_INVALID_SLOT) {
		struct atm_pool_slot *const s = &atm_mem_pool[next_free];
		atm_pool_free_list_head = s->next;
		s->next = next;
	}

	return next_free;
}

atm_pool_slot_idx_t atm_pool_alloc(uint8_t slots, atm_pool_slot_idx_t *dst)
{
	const atm_pool_slot_idx_t retval = atm_pool_free_list_head;
	atm_pool_slot_idx_t next_free = atm_pool_free_list_head;
	while (next_free != ATM_POOL_INVALID_SLOT) {
		if (dst) {
			*dst++ = next_free;
		}
		const atm_pool_slot_idx_t t = next_free;
		next_free = atm_mem_pool[next_free].next;
		if (!--slots) {
			atm_mem_pool[t].next = ATM_POOL_INVALID_SLOT;
			break;
		}
	}

	if (slots) {
		return ATM_POOL_INVALID_SLOT;
	}

	atm_pool_free_list_head = next_free;
	return retval;
}

void atm_pool_free(atm_pool_slot_idx_t slot)
{
	if (slot >= atm_mem_slot_count) {
		return;
	}
	atm_mem_pool[slot].next = atm_pool_free_list_head;
	atm_pool_free_list_head = slot;
}

void atm_pool_free_list1(atm_pool_slot_idx_t list_head)
{
	if (list_head >= atm_mem_slot_count) {
		return;
	}

	struct atm_pool_slot *next_free = &atm_mem_pool[list_head];
	while (next_free->next != ATM_POOL_INVALID_SLOT) {
		next_free = &atm_mem_pool[next_free->next];
	}
	next_free->next = atm_pool_free_list_head;
	atm_pool_free_list_head = list_head;
}

void atm_pool_free_list2(atm_pool_slot_idx_t list_head, atm_pool_slot_idx_t list_tail)
{
	atm_mem_pool[list_tail].next = atm_pool_free_list_head;
	atm_pool_free_list_head = list_head;
}

struct atm_pool_slot *atm_pool_idx2slot_ptr(atm_pool_slot_idx_t slot)
{
	return slot == ATM_POOL_INVALID_SLOT ? NULL : &atm_mem_pool[slot];
}

void *atm_pool_idx2data_ptr(atm_pool_slot_idx_t slot)
{
	return slot == ATM_POOL_INVALID_SLOT ? NULL : &atm_mem_pool[slot].data;
}

atm_pool_slot_idx_t atm_pool_slot_ptr2idx(const struct atm_pool_slot *const ptr)
{
	return ptr - atm_mem_pool;
}

atm_pool_slot_idx_t atm_pool_slot_from_dataptr(const void *const data_ptr)
{
	return (container_of((union atm_slot *)(data_ptr), struct atm_pool_slot, data) - atm_mem_pool);
}

__attribute__((weak)) void atm_pool_trace(bool allocating, atm_pool_slot_idx_t slot)
{

}