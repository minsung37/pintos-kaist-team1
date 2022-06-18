/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include <bitmap.h>
#include "threads/synch.h"

#define DISK_SECTOR_SIZE 512
#define SECTORS_PER_PAGE 8

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

struct bitmap *swap_table;
struct lock swap_lock;

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk.
	 * Implement a data structure to manage free and used areas in the swap disk.
	 * The swap area will be also managed at the granularity of PGSIZE (4096 bytes) */

	swap_disk = disk_get (1, 1);
	swap_table = bitmap_create (disk_size(swap_disk) / SECTORS_PER_PAGE);
	if (swap_table == NULL) {
		PANIC ("FAIL TO SET UP SWAP TABLE!");
	}
	lock_init(&swap_lock);
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;
	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
	size_t i;

	// lock_acquire(&swap_lock);
	bitmap_flip (swap_table, page->swap_idx);
	for (i = 0; i < SECTORS_PER_PAGE; i++) {
		disk_read (swap_disk, SECTORS_PER_PAGE * page->swap_idx + i, kva + DISK_SECTOR_SIZE * i);
	}
	// lock_release(&swap_lock);

	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	size_t idx = bitmap_scan (swap_table, 0, 1, false);
	size_t i;

	// lock_acquire(&swap_lock);

	bitmap_flip(swap_table, idx);

	// if (idx == (unsigned long)18446744073709551615UL)
	// 	return false;

	for (i = 0; i < SECTORS_PER_PAGE; i++) {
		disk_write (swap_disk, SECTORS_PER_PAGE * idx + i, page->frame->kva + DISK_SECTOR_SIZE * i);
	}
	// puts("swap_out");

	// bitmap_dump(swap_table);
	page->swap_idx = idx;
	pml4_clear_page (page->thread->pml4, page->va);

	// lock_release(&swap_lock);

	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	free (anon_page);
}
