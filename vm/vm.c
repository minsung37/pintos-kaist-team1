/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "threads/thread.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "hash.h"
#include "threads/thread.h"
#include "devices/timer.h"
#include "threads/vaddr.h"


#define list_elem_to_hash_elem(LIST_ELEM)                       \
	list_entry(LIST_ELEM, struct hash_elem, list_elem)

/* ------------------ project3 -------------------- */
static struct list frame_table;
/* ------------------------------------------------ */


/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();


#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	/* ------------------ project3 -------------------- */
	// 확실하지 않음
	// 프레임리스트 init
	list_init(&frame_table);
	/* ------------------------------------------------ */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initializer according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		
		// page = malloc (size of struct page);
		if (!vm_claim_page (upage)) {
			goto err;
		}
		struct page *newpage = spt_find_page (spt, upage);
		bool *initializer;
		switch (type) {
			// case VM_UNINIT:
			// 	initializer;
			// 	break;
			case VM_ANON:
				initializer = &anon_initializer;
				break;
			case VM_FILE:
				initializer = &file_backed_initializer;
				break;
			default:
				goto err;
		}
		uninit_new (newpage, upage, init, type, aux, initializer);
		/* TODO: Insert the page into the spt. */
		if (!spt_insert_page (spt, newpage)) {
			goto err;
		}

		return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
// spt_find가 이상함....
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page;
	struct hash_elem *e;
	/* TODO: Fill this function. */
  	page->va = pg_round_down(va);
  	struct hash_elem *e = hash_find (&spt->hash_table, &page->h_elem);
  	return e != NULL ? hash_entry (e, struct page, h_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	/* TODO: Fill this function. */
	struct hash_elem *e = hash_insert (&spt->hash_table, &page->h_elem);
	if (e == NULL) {
		return true;
	}
	return false;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	hash_delete(&spt->hash_table, &page->h_elem);
	vm_dealloc_page (page);
	
	return true;
}


/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	// 1. frame 할당
	frame = calloc(1, sizeof (struct frame));
	frame->kva = palloc_get_page(PAL_USER);
	if (frame->kva == NULL)
		PANIC("todo");
	// 2. 모든 프레임이 차있어서 할당 실패시
	// frame list 순회하면서 타겟 프레임 제거(해야함 앞으로 나중에 lru?)
	// swap out 아직 미완성으로 인해 나중에 할 것.
	// list_push_back(&frame_table, &frame->f_elem);
	// if (list_size(&frame_table) == MAX_FRAME_SIZE) {
	// 	clock_alg();
	// }
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* pseudo code for clock algorithm */
// void clock_alg (struct page *swapped_in_page) {
// 	???? 시침이 언제 돌아야하나 알아보기
// 	if cur_idx %= FRAME_TABLE_MAX;
// 	if (frame_table[cur_idx]->reference_bit == 1) {
// 		frame_table[cur_idx]->reference_bit--;

// 	}
// 	else {
// 		swap_out (frame_table[cur_idx]);
// 		swap_in (swapped_in_page);
// 		frame_table[cur_idx] = swapped_in_page;
// 		swapped_in_page->reference_bit++;

// 	}
// 	cur_idx++;
// }


/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	// check_address(addr);
	page = spt_find_page (spt, addr);

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page;
	/* TODO: Fill this function */
	// page = palloc_get_page(PAL_USER);
	page = malloc (sizeof (struct page));
	page->va = pg_round_down(va);
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	spt_insert_page (&thread_current ()->spt, page);

	return swap_in (page, frame->kva);
}

/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
  const struct page *p = hash_entry (p_, struct page, h_elem);
  return hash_bytes (&p->va, sizeof p->va);
}

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_,
           const struct hash_elem *b_, void *aux UNUSED) {
  const struct page *a = hash_entry (a_, struct page, h_elem);
  const struct page *b = hash_entry (b_, struct page, h_elem);

  return a->va < b->va;
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init (&spt->hash_table, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */

}