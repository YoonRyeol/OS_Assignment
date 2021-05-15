#include <mem/palloc.h>
#include <bitmap.h>
#include <type.h>
#include <round.h>
#include <mem/mm.h>
#include <synch.h>
#include <device/console.h>
#include <mem/paging.h>
#include<mem/swap.h>

/* Page allocator.  Hands out memory in page-size (or
   page-multiple) chunks.  
   */
/* pool for memory */
struct memory_pool
{
	struct lock lock;                   
	struct bitmap *bitmap; 
	uint32_t *addr;                    
};
/* kernel heap page struct */
struct khpage{
	uint16_t page_type;
	uint16_t nalloc;
	uint32_t used_bit[4];
	struct khpage *next;
};

/* free list */
struct freelist{
	struct khpage *list;
	int nfree;
};

static struct khpage *khpage_list;
static struct freelist freelist;
static uint32_t kerenl_alloc_index;
static uint32_t user_alloc_index;

static struct memory_pool kernel_pool;
static struct memory_pool user_pool;

/* Initializes the page allocator. */
//
	void
init_palloc (void) 
{
	//커널 메모리 풀을 생성하고 초기화한다.
	kernel_pool.addr = (uint32_t *) KERNEL_ADDR;
	size_t kernel_bitmap_size = bitmap_struct_size(USER_POOL_START-KERNEL_ADDR);
	uint32_t start_point = KERNEL_ADDR;
	kernel_pool.bitmap = create_bitmap(kernel_bitmap_size, start_point, kernel_bitmap_size);
	//유저 메모리 풀을 생성하고 초기화 한다.
	user_pool.addr = (uint32_t *) USER_POOL_START;
	size_t user_bitmap_size = bitmap_struct_size(RKERNEL_HEAP_START-USER_POOL_START);
	user_pool.bitmap = create_bitmap(user_bitmap_size, start_point+kernel_bitmap_size, user_bitmap_size);

}



/* 
Obtains and returns a group of PAGE_CNT contiguous free pages.
flags인자를 추가해주었다.
*/
	uint32_t *
palloc_get_multiple_page (enum palloc_flags flags, size_t page_cnt)
{
// 페이지를 할당하고자 하는 메모리 영역에 따라 변수를 초기화
//	printk("here\n");
	uint32_t target_addr;
	uint32_t *page_alloc_index;
	struct bitmap *target_bitmap;
	if(flags == kernel_area){
		target_addr = KERNEL_ADDR;
		target_bitmap = kernel_pool.bitmap;
		page_alloc_index = &kerenl_alloc_index;
//		printk("-------kernel area\n");
	}
	else if(flags == user_area){
		target_addr = USER_POOL_START;
		target_bitmap = user_pool.bitmap;
		page_alloc_index = &user_alloc_index;
	}

	void *pages = NULL;

	if (page_cnt == 0)
		return NULL;
	uint32_t new_page_index;
// 비트맵의 1번지 부터 page_cnt만큼의 연속된 빈 자리 (1로 초기화 된)가 있는지 확인하고 있다면 0으로 바꾸어
// 할당되었음을 표시한다. 그리고 new_page_index로 그 위치의 인덱스를 받는다.
	new_page_index = find_set_bitmap(target_bitmap, 1, page_cnt, false);
//	printk("address : %x\n", target_addr + PAGE_SIZE*new_page_index);
	pages = target_addr + PAGE_SIZE * new_page_index;

	if(pages == NULL){
		//페이지 주소를 생성, 페이지 폴트가 일어날 수 있음
		pages = (void*)(target_addr + new_page_index * PAGE_SIZE);
	}

	if (pages != NULL) 
	{
		//memeset()으로 초기화
		memset (pages, 0, PAGE_SIZE * page_cnt);
	}
//	printk("page index : %d, page table index : %d\n", pde_idx_addr(pages), pte_idx_addr(pages));
	return (uint32_t*)pages; 
}

/* 
Obtains a single free page and returns its address.
flags 인자를 추가해 주었다.
*/
	uint32_t *
palloc_get_one_page (enum palloc_flags flags) 
{
	return palloc_get_multiple_page (flags, 1);
}

/* 
Frees the PAGE_CNT pages starting at PAGES. 
*/
	void
palloc_free_multiple_page (enum palloc_flags flags, void *pages, size_t page_cnt) 
{
	uint32_t target_addr;
	uint32_t *page_alloc_index;
	struct bitmap *target_bitmap;
	//해제하고자 하는 페이지가 위치한 메모리영역에 따라 변수들을 초기화한다.
	if(flags == kernel_area){
		target_addr = KERNEL_ADDR;
		target_bitmap = kernel_pool.bitmap;
		page_alloc_index = &kerenl_alloc_index;
	}
	else if(flags == user_area){
		target_addr = USER_POOL_START;
		target_bitmap = user_pool.bitmap;
		page_alloc_index = &user_alloc_index;
	}

	//비트맵에서의 인덱스를 찾아냄
	size_t page_idx = (((uint32_t)pages - target_addr) / PAGE_SIZE);
	//인덱스로 부터 page_cnt 개수의 맵을 1로 초기화 하여 할당이 되어있지 않음을 알린다.
	find_set_bitmap(target_bitmap, page_idx, page_cnt, true);

}

/* 
Frees the page at PAGE.
flags 인자를 추가해 주었다.
*/
	void
palloc_free_one_page (enum palloc_flags flags, void *page) 
{
	palloc_free_multiple_page (flags, page, 1);
}


void palloc_pf_test(void)
{
	//유저 에리어 옵션 넣어주어야 함.
	uint32_t *one_page1 = palloc_get_one_page(user_area);
	uint32_t *one_page2 = palloc_get_one_page(user_area);
	uint32_t *two_page1 = palloc_get_multiple_page(user_area,2);
	uint32_t *three_page;
	printk("one_page1 = %x\n", one_page1); 
	printk("one_page2 = %x\n", one_page2); 
	printk("two_page1 = %x\n", two_page1);

	printk("=----------------------------------=\n");
	palloc_free_one_page(user_area, one_page1);
	palloc_free_one_page(user_area, one_page2);
	palloc_free_multiple_page(user_area, two_page1,2);
	one_page1 = palloc_get_one_page(user_area);
	one_page2 = palloc_get_one_page(user_area);
	two_page1 = palloc_get_multiple_page(user_area,2);

	printk("one_page1 = %x\n", one_page1);
	printk("one_page2 = %x\n", one_page2);
	printk("two_page1 = %x\n", two_page1);

	printk("=----------------------------------=\n");
	palloc_free_multiple_page(user_area, one_page2, 3);
	one_page2 = palloc_get_one_page(user_area);
	three_page = palloc_get_multiple_page(user_area,3);
 
	printk("one_page1 = %x\n", one_page1);
	printk("one_page2 = %x\n", one_page2);
	printk("three_page = %x\n", three_page);

	palloc_free_one_page(user_area,one_page1);
	palloc_free_one_page(user_area, three_page);
	three_page = (uint32_t*)((uint32_t)three_page + 0x1000);
	palloc_free_one_page(user_area, three_page);
	three_page = (uint32_t*)((uint32_t)three_page + 0x1000);
	palloc_free_one_page(user_area, three_page);
	palloc_free_one_page(user_area, one_page2);
}
