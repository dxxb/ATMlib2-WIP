
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define ATM_POOL_INVALID_SLOT (255)

typedef uint8_t atm_pool_slot_idx_t;

struct atm_pool_slot;

extern const uint8_t atm_mem_slot_count;
extern atm_pool_slot_idx_t atm_pool_free_list_head;

#define ATM_MEM_POOL(mem_slots_count) \
	const uint8_t atm_mem_slot_count = (mem_slots_count); \
	atm_pool_slot_idx_t atm_pool_free_list_head; \
	struct atm_pool_slot atm_mem_pool[(mem_slots_count)];

#ifndef container_of
#define container_of(ptr, type, member) ({ \
                const typeof( ((type *)0)->member ) *__mptr = (ptr);  \
                (type *)( (char *)__mptr - offsetof(type,member) );})
#endif /* container_of */

#define atm_pool_slotptr_from_dataptr(data_ptr) container_of((union atm_slot *)(data_ptr), struct atm_pool_slot, data)
#define atm_pool_next_slot_from_dataptr(data_ptr) container_of((union atm_slot *)(data_ptr), struct atm_pool_slot, data)->next

void atm_pool_init(void);
atm_pool_slot_idx_t atm_pool_alloc1(atm_pool_slot_idx_t next);
atm_pool_slot_idx_t atm_pool_alloc(uint8_t slots, atm_pool_slot_idx_t *dst);
void atm_pool_free(atm_pool_slot_idx_t slot);
void atm_pool_free_list1(atm_pool_slot_idx_t list_head);
void atm_pool_free_list2(atm_pool_slot_idx_t list_head, atm_pool_slot_idx_t list_tail);
struct atm_pool_slot *atm_pool_idx2slot_ptr(atm_pool_slot_idx_t slot);
void *atm_pool_idx2data_ptr(atm_pool_slot_idx_t slot);
atm_pool_slot_idx_t atm_pool_slot_ptr2idx(const struct atm_pool_slot *const ptr);
atm_pool_slot_idx_t atm_pool_slot_from_dataptr(const void *const ptr);
