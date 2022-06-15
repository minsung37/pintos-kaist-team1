/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
static bool
lazy_load_segment(struct page *page, struct file_info *aux)
{
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */

	if (file_read_at (aux->file, page->frame->kva, aux->read_bytes, aux->ofs) != aux->read_bytes) {
		vm_dealloc_page (page);
		// free (aux);

		return false;
	}
	memset (page->frame->kva + aux->read_bytes, 0, aux->zero_bytes);
	// free (aux);

	return true;

}


/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) {

	// ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
	// ASSERT(pg_ofs(upage) == 0);
	// ASSERT(ofs % PGSIZE == 0);

	void *mmap_addr = addr;

	struct file * mmap_file = file_reopen (file);
	size_t read_bytes = file_length(file) / PGSIZE + file_length(file) % PGSIZE;
	size_t zero_bytes = PGSIZE - file_length(file) % PGSIZE;
	

	while (read_bytes > 0 || zero_bytes > 0)
	{
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		struct file_info *f_info = (struct file_info *) calloc (1, sizeof (struct file_info));

		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		f_info->file = mmap_file;
		f_info->ofs = offset;
		f_info->read_bytes = page_read_bytes;
		f_info->zero_bytes = page_zero_bytes;

		if (!vm_alloc_page_with_initializer(VM_FILE, offset, writable, lazy_load_segment, f_info)){
			return false;
		}


		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
	}

	return mmap_addr;
}


/* Do the munmap */
void
do_munmap (void *addr) {
}
