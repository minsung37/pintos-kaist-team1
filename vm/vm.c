/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "hash.h"
#include <bitmap.h>

#include "devices/timer.h"

#include <string.h>

/* ------------------ project3 -------------------- */
static struct list victim_list;
static struct lock victim_lock;
/* ------------------------------------------------ */

#define MAX_STACK_SIZE (1 << 20)

// #define SWAP_DISK_SECTORS 2016
// #define SECTORS_PER_PAGE 8
// #define SWAP_SLOT_CNT ((SWAP_DISK_SECTORS) / (SECTORS_PER_PAGE))


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
	list_init(&victim_list);
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
		
	struct supplemental_page_table *spt = &thread_current ()->spt;
	struct page *newpage = (struct page *) malloc (sizeof (struct page));

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initializer according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		if (type & VM_FORK) {

			struct page *parent_p = (struct page *) aux;
			struct page *child_p = newpage;
			
			memcpy (child_p, parent_p, sizeof (struct page));

			if (parent_p->frame != NULL) {

				struct frame *child_f = vm_get_frame ();

				/* Set links */
				child_f->page = child_p;
				child_p->frame = child_f;

				memcpy (child_f->kva, parent_p->frame->kva, PGSIZE);
				pml4_set_page (thread_current ()->pml4, child_p->va, child_f->kva, child_p->writable);
			}

			if (!spt_insert_page (spt, child_p)) {
				goto err;
			}
			
			return true;
		}

		switch (VM_TYPE(type)) {
			case VM_ANON:
				uninit_new (newpage, upage, init, type, aux, anon_initializer);
				break;
			case VM_FILE:
				uninit_new (newpage, upage, init, type, aux, file_backed_initializer);
				newpage->is_mmapped = true;
				break;
			default:
				goto err;
		}

		if (type & VM_STACK) {
			newpage->va = upage;
			newpage->writable = writable;
			if (!vm_do_claim_page (newpage)) {
				goto err;
			}
		}
		/* TODO: Insert the page into the spt. */
		newpage->writable = writable;

		if (!spt_insert_page (spt, newpage)) {
			goto err;
		}

		newpage->thread = thread_current ();

		return true;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page p;
	struct hash_elem *e;
	/* TODO: Fill this function. */
  	p.va = pg_round_down(va);
  	e = hash_find (&spt->hash_table, &p.h_elem);
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
	struct frame *victim;
	 /* TODO: The policy for eviction is up to you. */
	struct page *victim_page;
	struct list_elem *e;
	/* clock algorithm */
	// if (!list_empty( &victim_list)) {
	
	ASSERT(!list_empty(&victim_list));

	for (;;) {
		e = list_pop_front (&victim_list);
		struct page *victim_page = list_entry (e, struct page, v_elem);

		if (pml4_is_accessed (victim_page->thread->pml4, victim_page->va)) {

			pml4_set_accessed (victim_page->thread->pml4, victim_page->va, false);
			list_push_back (&victim_list, e);
		}
		else {

			victim = victim_page->frame;

			return victim;
		}
	}
	// lock_release(&victim_lock);
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	/* TODO: swap out the victim and return the evicted frame. */
	// PANIC("todo");
	// lock_acquire(&victim_lock);
	struct frame *victim = vm_get_victim ();

	swap_out (victim->page);
	// lock_release(&victim_lock);

	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space. */
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	// 1. frame 할당 calloc으로 하면 안됨.
	frame = malloc (sizeof (struct frame));
	frame->page = NULL;
	frame->kva = palloc_get_page (PAL_USER | PAL_ZERO);

	if (frame->kva == NULL) {
		struct frame *victim = vm_evict_frame ();
		// palloc_free_page (victim->kva);
		memset(victim->kva, 0, PGSIZE);
		frame->kva = victim->kva;
		// frame->kva = palloc_get_page (PAL_USER | PAL_ZERO);
	}

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}


/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	if (vm_alloc_page (VM_ANON | VM_STACK, pg_round_down(addr), true)) {
		thread_current ()->stack_bottom -= PGSIZE;
	}
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {

	struct thread *current = thread_current ();
	struct supplemental_page_table *spt UNUSED = &current->spt;
	struct page *page = NULL;

	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	if (not_present) {

		page = spt_find_page (spt, addr);
		if (page == NULL) {
			if (addr < USER_STACK - MAX_STACK_SIZE || addr > USER_STACK) {
				return false;
			}
			/********/
			void *curr_rsp = is_kernel_vaddr(f->rsp) ? current->rsp : f->rsp;
			if (curr_rsp - 8 <= addr) {
				while (current->stack_bottom > addr) {
					vm_stack_growth(thread_current()->stack_bottom - PGSIZE);
				}
				return true;
			}
			else {
				return false;
			}
			/********/
		}

	}

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
	page = spt_find_page (&thread_current ()->spt, pg_round_down (va));
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {

	if (page == NULL)
		return false;
	
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	pml4_set_page (thread_current ()->pml4, page->va, frame->kva, page->writable);
	list_push_back (&victim_list, &page->v_elem);

	return swap_in (page, frame->kva);		// uninit_initialize (page, frame->kva)
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
	
	int i;

	for (i = 0; i < src->hash_table.bucket_cnt; i++) {
		struct list *bucket = &src->hash_table.buckets[i];
		struct list_elem *elem, *next;

		for (elem = list_begin (bucket); elem != list_end (bucket); elem = next) {
			next = list_next (elem);

			struct hash_elem *h_elem = list_entry(elem, struct hash_elem, list_elem);
			
			struct page *p = hash_entry(h_elem, struct page, h_elem);

			if (!vm_alloc_page_with_initializer (page_get_type(p) | VM_FORK, p->va, p->writable, NULL, p)) {
				return false;
			}
		}
	}
	return true;

	// 보라
	// struct hash_iterator i;

	// hash_first (&i, &src->hash_table);
	// while (hash_next (&i))
	// {
	// 	struct page *p = hash_entry (hash_cur (&i), struct page, h_elem);
	// 	if (!vm_alloc_page_with_initializer (page_get_type(p) | VM_FORK, p->va, p->writable, NULL, p)) {
	// 		return false;
	// 	}
	// }
	// return true;

}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	
	// int i;

	// for (i = 0; i < spt->hash_table.bucket_cnt; i++) {
	// 	struct list *bucket = &spt->hash_table.buckets[i];
	// 	struct list_elem *elem, *next;

	// 	for (elem = list_begin (bucket); elem != list_end (bucket); elem = next) {
	// 		next = list_next (elem);
	// 		struct hash_elem *h_elem = list_entry(elem, struct hash_elem, list_elem);
	// 		struct page *p = hash_entry(h_elem, struct page, h_elem);

	// 		destroy(p);
	// 	}
	// }
	//하면 에러남???
	//
	hash_clear (&spt->hash_table, NULL);
	// hash_destroy (&spt->hash_table, NULL);
}

