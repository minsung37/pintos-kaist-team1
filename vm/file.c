/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"

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

// file_backed_initializer (page, VM_FILE, frame->kva) /*page already has been claimed.
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
	struct file_info *f_info = (struct file_info *) page->uninit.aux;

	file_page->file = f_info->file;
	file_page->ofs = f_info->ofs;
	file_page->read_bytes = f_info->read_bytes;
	file_page->zero_bytes = f_info->zero_bytes;

	if (file_read_at (f_info->file, kva, f_info->read_bytes, f_info->ofs) != f_info->read_bytes) {
		vm_dealloc_page (page);

		return false;
	}

	memset (kva + f_info->read_bytes, 0, f_info->zero_bytes);
	
	return true;
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
void *
do_mmap (void *addr, size_t length, int writable,
        struct file *file, off_t offset) {
    /* Maps length bytes the file open as fd 
     * starting from offset byte into the process's virtual address space at addr. */
	// printf("do_mmap starts!\n");

    struct thread *current = thread_current ();
    // struct mmap_file *mfile = (struct mmap_file *) malloc(sizeof (struct mmap_file));
	// printf("file_length %d, length %d\n", file_length(file), length);
	size_t f_length = file_length(file);
    // mfile->file = file;
    // mfile->sa = addr;
    // list_push_back (&current->mfile_list, &mfile->mf_elem);
	// printf("from %d in file_length %d\n", offset, length);
    void *curr = addr;
    while (f_length > 0)
    {
        /* Do calculate how to fill this page.
         * We will read PAGE_READ_BYTES bytes from FILE
         * and zero the final PAGE_ZERO_BYTES bytes. */
        struct file_info *f_info = (struct file_info *) calloc (1, sizeof (struct file_info));

        size_t page_length = f_length < PGSIZE ? f_length : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_length;

        /* TODO: Set up aux to pass information to the lazy_load_segment. */
        f_info->file = file;
        f_info->ofs = offset;
        f_info->read_bytes = page_length;
        f_info->zero_bytes = page_zero_bytes;

        if (!vm_alloc_page_with_initializer(VM_FILE, curr,
                                            writable, NULL, f_info))
            return NULL;

        /* Advance. */
        f_length -= page_length;
        curr += PGSIZE;
        offset += page_length;
    }
    return addr;

}

/* Do the munmap */
void
do_munmap (void *addr) {

    struct supplemental_page_table *spt = &thread_current ()->spt;
    void *curr = addr;
	// printf("do_MUNmap call! ismapped? %d\n", (spt_find_page(spt, addr)->is_mmapped));
	// printf("do_MUNmap call! isnull? %d\n", (spt_find_page(spt, addr) == NULL) || (!spt_find_page(spt, addr)->is_mmapped));
    struct file *f = spt_find_page(spt, curr)->file.file;

    for (curr = addr; (spt_find_page(spt, curr) != NULL); curr += PGSIZE) {
        struct page *p = spt_find_page(spt, curr);
        // struct file_page *fp = &p->file;
        // printf("f %p, fp_file %p\n", f, p->file.file);
        if (!p->is_mmapped || p->file.file != f)
            return;
        
        // printf("11 do_MUNmap call! page %d\n", (spt_find_page(spt, curr)));
        // printf("current addr %p, dirty bit? %d\n", curr, pml4_is_dirty(thread_current ()->pml4, curr));
        if (pml4_is_dirty(thread_current ()->pml4, curr)) {
	        // printf("22 do_MUNmap call! page %d\n", (spt_find_page(spt, curr)));

            file_write_at (p->file.file, curr, p->file.read_bytes, p->file.ofs);
	        // printf("33 do_MUNmap call! page %d\n", (spt_find_page(spt, curr)));

            pml4_set_dirty(thread_current ()->pml4, curr, false);
        }
	    // printf("44 do_MUNmap call! page %d\n", (spt_find_page(spt, curr)));

        p->is_mmapped = false;
        // vm_dealloc_page(p);
        destroy (p);
	    // printf("55 do_MUNmap call! page %d\n", (spt_find_page(spt, curr)));
    }
	// printf("do_MUNmap finished!\n");
}

