/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "threads/mmu.h"
#include "vm/inspect.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void vm_init(void)
{
	vm_anon_init();
	vm_file_init();
#ifdef EFILESYS /* For project 4 */
	pagecache_init();
#endif
	register_inspect_intr();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type(struct page *page)
{
	int ty = VM_TYPE(page->operations->type);
	switch (ty)
	{
	case VM_UNINIT:
		return VM_TYPE(page->uninit.type);
	default:
		return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
									vm_initializer *init, void *aux)
{

	ASSERT(VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page(spt, upage) == NULL)
	{
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		/* 페이지를 생성하고, */
		struct page *p = (struct page *)malloc(sizeof(struct page));
		/* VM 유형에 따라 초기화 함수를 가져와서 */
		bool (*page_initializer)(struct page *, enum vm_type, void *);

		switch (VM_TYPE(type))
		{
		case VM_ANON:
			page_initializer = anon_initializer;
			break;
		case VM_FILE:
			page_initializer = file_backed_initializer;
			break;
		}
		/* uninit_new를 호출해 "uninit" 페이지 구조체를 생성하세요. */
		uninit_new(p, upage, init, type, aux, page_initializer);
		/* uninit_new를 호출한 후에는 필드를 수정해야 합니다. */
		p->writable = writable;

		/* TODO: Insert the page into the spt. */
		return spt_insert_page(spt, p);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
/* spt에서 va에 해당하는 page를 찾아서 반환 */
struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{
	struct page *page = NULL;
	/* TODO: Fill this function. */
	page = (struct page *)malloc(sizeof(struct page));
	struct hash_elem *e;

	/* va에 해당하는 hash_elem 찾기 */
	page->va = pg_round_down(va); // page의 시작 주소 할당
	e = hash_find(&spt->spt_hash, &page->hash_elem);
	free(page);

	/* 있으면 e에 해당하는 페이지 반환 */
	return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	int succ = false;
	/* TODO: Fill this function. */
	// 존재하지 않을 경우에만 삽입
	return hash_insert(&spt->spt_hash, &page->hash_elem) == NULL ? true : false;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
	vm_dealloc_page(page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim(void)
{
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void)
{
	struct frame *victim UNUSED = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame(void)
{
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	/* user pool에서 새로운 physical page를 가져온다. */
	void *kva = palloc_get_page(PAL_USER);

	if (kva == NULL)   // page 할당 실패
		PANIC("TODO"); // 나중에 swap_out 처리

	frame = (struct frame *)malloc(sizeof(struct frame)); // 프레임 할당
	frame->kva = kva;									  // 프레임 멤버 초기화
	frame->page = NULL;

	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth(void *addr UNUSED)
{
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED)
{
}

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
						 bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
{
	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
	struct page *page = NULL;

	if (addr == NULL)
		return false;

	if (is_kernel_vaddr(addr))
		return false;

	/* 접근한 메모리의 physical page가 존재하지 않은 경우 */
	if (not_present)
	{
		/* TODO: Validate the fault */
		/* TODO: Your code goes here */
		page = spt_find_page(spt, addr);
		if (page == NULL)
			return false;
		if (write == 1 && page->writable == 0) // write 불가능한 페이지에 write 요청한 경우
			return false;
		return vm_do_claim_page(page);
	}
	return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page)
{
	destroy(page);
	free(page);
}

/* Claim the page that allocate on VA. */
/* va로 page를 찾아서 vm_do_claim_page를 호출하는 함수 */
bool vm_claim_page(void *va UNUSED)
{
	struct page *page = NULL;
	/* TODO: Fill this function */
	page = spt_find_page(&thread_current()->spt, va);
	if (page == NULL)
		return false;

	return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{
	struct frame *frame = vm_get_frame();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* 가상 주소와 물리 주소를 매핑 */
	struct thread *current = thread_current();
	pml4_set_page(current->pml4, page->va, frame->kva, page->writable);

	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
	hash_init(&spt->spt_hash, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
/* SPT를 복사하는 함수 (자식 프로세스가 부모 프로세스의 실행 컨텍스트를 상속해야 할 때 (즉, fork() 시스템 호출이 사용될 때) 사용) */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
								  struct supplemental_page_table *src UNUSED)
{
	// TODO: 보조 페이지 테이블을 src에서 dst로 복사합니다.
	// TODO: src의 각 페이지를 순회하고 dst에 해당 entry의 사본을 만듭니다.
	// TODO: uninit page를 할당하고 그것을 즉시 claim해야 합니다.
	struct hash_iterator i;
	hash_first(&i, &src->spt_hash);
	while (hash_next(&i))
	{
		/* src_page 정보 */
		struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
		enum vm_type type = src_page->operations->type;
		void *upage = src_page->va;
		bool writable = src_page->writable;

		/* type이 uninit이면 */
		if (type == VM_UNINIT)
		{ /* uninit page 생성 & 초기화 */
			vm_initializer *init = src_page->uninit.init;
			void *aux = src_page->uninit.aux;
			vm_alloc_page_with_initializer(VM_ANON, upage, writable, init, aux);
			continue;
		}

		/* type이 anon이면 */
		if (!vm_alloc_page(type, upage, writable)) // uninit page 생성 & 초기화
			return false;						   // init이랑 aux는 Lazy Loading에 필요. 지금 만드는 페이지는 기다리지 않고 바로 내용을 넣어줄 것이므로 필요 없음

		/* vm_claim_page으로 요청해서 매핑 & 페이지 타입에 맞게 초기화 */
		if (!vm_claim_page(upage))
			return false;

		/* 매핑된 프레임에 내용 로딩 */
		struct page *dst_page = spt_find_page(dst, upage);
		memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
	}
	return true;
}

/* Free the resource hold by the supplemental page table */
/* SPT가 보유하고 있던 모든 리소스를 해제하는 함수 (process_exit(), process_cleanup()에서 호출) */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	/* 페이지 항목들을 순회하며 테이블 내의 페이지들에 대해 destroy(page)를 호출 */
	hash_clear(&spt->spt_hash, hash_page_destroy); // 해시 테이블의 모든 요소를 제거

	/** hash_destroy가 아닌 hash_clear를 사용해야 하는 이유
	 * 여기서 hash_destroy 함수를 사용하면 hash가 사용하던 메모리(hash->bucket) 자체도 반환한다.
	 * process가 실행될 때 hash table을 생성한 이후에 process_clean()이 호출되는데,
	 * 이때는 hash table은 남겨두고 안의 요소들만 제거되어야 한다.
	 * 따라서, hash의 요소들만 제거하는 hash_clear를 사용해야 한다.
	 */
}

void hash_page_destroy(struct hash_elem *e, void *aux)
{
	struct page *page = hash_entry(e, struct page, hash_elem);
	destroy(page);
	free(page);
}

/* Returns a hash value for page p. */
unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED)
{
	const struct page *p = hash_entry(p_, struct page, hash_elem);
	return hash_bytes(&p->va, sizeof p->va);
}

/* Returns true if page a precedes page b. */
bool page_less(const struct hash_elem *a_,
			   const struct hash_elem *b_, void *aux UNUSED)
{
	const struct page *a = hash_entry(a_, struct page, hash_elem);
	const struct page *b = hash_entry(b_, struct page, hash_elem);

	return a->va < b->va;
}