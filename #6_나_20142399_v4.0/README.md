# 페이지 할당자

## 1. 개요

본 과제는 Memory Page Allocator를 구현하는 것을 목표로 하는 것이다. 본래 목적은 가상메모리를 위한 Page Allocator를 구현하는 것이지만 이번 과제에서는 실제 메모를 주소를 가상메모리를 관리하는 방식으로 관리하는 page allocator를 구현하게 된다. 따라서 메모리의 실주소를 page directory와 page table을 통하여 관리한다. 

 페이징은 본래 가상 메모리를 모두 같은 크기의 블록으로 관리하는 기법이다. 일정한 크기를 가진 block을 page라고 하며 실제 메모리의 블록을 page frame이라고 한다. SSUOS는 가상메모리를 사용하지 않기에 page가 곧 page frame이 된다. 가상 메모리의 구현은 가상 메모리가 참조하는 page 주소를 실 메모리 page frame 주소로 변환하는 것인데 SSUOS는 가상 메모리가 없기에 주소 변환의 과정이 빠지게 된다. SSUOS의 page frame의 크기는 기본 4KB로 설정되어있다.
 
 본 과제는 SSUOS가 khpage list로 페이지를 관리하는 기본 방식을 비트맵을 사용하여 관리하도록 수정하는 것이 목표이다. 비트맵은 유닛으로 나타나는 한 메모리영역의 상태를 비트로 표현하는 방식이다. 메모리는 비트의 유닛으로 쪼개어진다. 유닛의 크기는 시스템에 따라 다를 수 있는데, SSUOS는 4KB를 한 비트로써 표현하고 있다. 각 메모리 유닛은 비트맵의 한 비트로 표현이 되는데 SSUOS에서는 1이면 그 유닛이 사용되지 않는 것이고 0이면 그 유닛이 사용되고 있는 것이다. 비트맵의 크기는 오직 메모리의 크기와 유닛의 크기에 결정되기 때문에 메모리의 상태를 나타내는데 상대적으로 편리하다.
 
 또한 본 과제는 page directory와 page table에 대한 공유기능을 지원하도록 커널을 수정해야 한다. page table은 가상 메모리 체계에서 사용된다. 물리적으로, 각 프로세스에게 할당된 메모리 영역은 여기저기에 흩어져 있을 수 있고, 또는 스왑영역에 존재할 수 있다. 한 프로세스가 이러한 메모리 영역에 접근하려고 요청할 때 OS는 프로세스에 의해 제공된 가상 메모리를 데이터가 저장된 실제 메모리 주소로 변환해야 한다. 페이지 테이블은 OS가 가상주소를 실제주소로 변환하는 mapping을 저장하는 공간이며 각 mapping은 page table entry라고 불린다.
 
 Page directory는 이러한 page table의 각 주소를 담고 있는 자료구조이다. 페이지 디렉토리는 페이지 디렉토리 엔트리의 집합이며 각 페이지 디렉토리 엔트리는 대응되는 주소를 가지는 페이지 테이블의 주소를 가지게 된다. 페이지 테이블로만 모든 메모리를 관리하면 페이지 테이블이 매우 커질 수 있는데 이러한 방식을 통하여 메모리별로 대응되는 페이지 테이블을 가지게 됨으로써 전체 메모리에 대한 페이지 테이블을 잘라내어 관리할 수 있기에 더욱 효율적인 메모리 관리를 가능하게 한다.
 
 ## 2. 상세설계
 
 ### 2-1. 구현 함수별 프로토타입 설계 및 기능 설명
 
 - #### palloc.c
 
 #### 1. static struct memory_pool kernel_pool, static struct memory_pool user_pool
 
 각 각 kernel 메모리 영역, user 메모리 영역을 관리하기 위한 memory_pool 구조체이다. 각 메모리의 base주소 및 bitmap의 정보를 담게된다. init_palloc()에서 초기화되며 각 영역의 bitmap은 palloc_get_multiple_page(), palloc_free_multiple_page()에서 관리된다. 


#### 2. static uint32_t kernel_alloc_index, static uint32_t user_alloc_index

 각 각 kernel 메모리 영역의 인덱스, user 메모리 영역의 인덱스를 관리하기 위한 변수이다. ini_palloc()에서 초기화 되어 palloc_get_multiple_page()에서 사용된다.
 

#### 3. void init_palloc(void)

 기존의 SSUOS는 할당되는 페이지를 khpage_list를 통하여 관리하기 때문에 khpage_list를 초기화시켜 주는 역할을 했다. 본 과제는 bitmap을 사용하여 할당되는 페이지를 관리해야 했기 때문에 기존의 khpage list를 초기화 하는 부분을 지우고 memory_pool을 초기화하고 bitmap을 생성하고 초기화 하는 코드를 넣어주었다.
 
 먼저 kerenl_pool, user_pool을 초기화한다. memory_pool의 addr 변수에는 각 메모리의 base 주소를 저장해주었다. 그리고 bitmap_struct_size()를 사용하여 생성해야 하는 bitmap의 크기와 bitmap 구조체의 크기를 더한 값을 뽑아내어 create_bitmap()에 인자로 넣어줌으로써 필요한 크기 만큼의 bitmap을 생성해준다. kernel_pool을 먼저 생성해 주었기 때문에 kernel_pool 이후의 영역에 user_pool을 똑같은 방식으로 생성해준다. 그리고 kernel_alloc_index와 user_alloc_index를 1로 초기화 해주면서 작업을 마친다.
 

#### 4. uint32_t *palloc_get_multiple_page(enum palloc_flag flags, size_t page_cnt)

 이 함수는 page_cnt 만큼의 개수의 페이지를 flags에 명세된 영역에 할당해 주는 함수이다. flags가 0이면 kernel area, 1이면 user area에 page_cnt 만큼의 개수를 할당한다. 
 
 함수가 실행되면 목표 영역이 kernel area인지, user area인지부터 확인한다. 메모리의 base를 나타내는 target_addr, 현재 목표가 되는 비트맵을 나타내는 target_bitmap 변수를 각 영역에 맞도록 초기화를 한다.
 
그리고 비트맵에서 cnt만큼의 빈자리가 있는지 첫 번째 인덱스부터 확인하고 해당하는 영역이 있다면 그 영역의 비트맵 값을 할당되었음을 알리는 값으로 바꾸고 해당 비트맵의 시작하는 인덱스 값을 받는다. 다음을 나타내는 소스코드는 다음과 같다.


```
 	new_page_index = find_set_bitmap(target_bitmap, 1, page_cnt, false);
```

find_set_bitmap()을 이용하여 인덱스 번호 1부터 false가 연속으로 page_cnt가 있는 영역을 target_bitmap으로부터 찾아 있다면 그 영역의 시작 인덱스를 new_page_index에 저장하게 된다.

 그 후 해당 영역의 실제 주소값을 다음 식 
 
 ```
	pages = target_addr + PAGE_SIZE * new_page_index;
 ```

을 사용하여 pages에 저장하게 된다.

 이후 해당 주소의 메모리를 참조하게 되는데 만약 현재의 페이지 테이블에 등록되지 않은 값이라면 페이지 폴트가 발생하게 된다. 페이지 폴트 핸들러를 통하여 해당 주소를 페이지 테이블에 등록하게 되고 memset()을 통하여 현재 할당된 페이지 영역을 0으로 초기화 하게된다. 그리고 첫 번째 페이지의 실주소를 리턴한다.
 
 

#### 5. uint32_t * palloc_get_one_page(enum palloc_flags flags)

 페이지를 단 한 개 할당시켜주는 함수이다. flags 인자를 추가해주었다.
 

#### 6. void palloc_free_multiple_page(enum palloc_flags flags, void *pages, size_t page_cnt)

 해당 함수는 할당된 페이지를 해제해주는 함수이다. flags로 해제하고 싶은 페이지의 영역, pages로 해제하고 싶은 페이지의 주소, page_cnt로 해제하고 싶은 페이지의 크기를 나타낸다. 
 
 먼저 flags로 작업하고 싶은 메모리의 영역이 어딘지 검사하고 target_addr에 작업하고 싶은 메모리의 base주소, target_bitmap에 작업하고 싶은 영역의 bitmap주소를 넣는다. 
 
 그 후 다음과 같은 식으로 작업하고 싶은 페이지의 인덱스를 계산한다. 
 
 ```
	size_t page_idx = (((uint32_t)pages - target_addr) / PAGE_SIZE);
 ```
 
 다음 식을 통해서 page_idx에 현재 작업하고자 하는 페이지의 비트맵에서의 첫 인덱스가 산출된다. 
 
 ```
	find_set_bitmap(target_bitmap, page_idx, page_cnt, true);
 ```
 
 그 뒤, 다음 명령을 통하여 현재 비트맵의 page_idx 번지부터 page_cnt만큼 비트맵의 값이 true인 영역을 찾아 false로 바꾼다. bitmap이 false가 됨으로써 대응되는 메모리가 할당되지 않았음을 나타낸다.
 

#### 7. void palloc_free_one_page (enum palloc_flags flags, void *page)

 flags 영역의 page주소에 해당하는 페이지를 한 개 해제시켜주는 함수이다. flags인자를 추가했다.



- #### paging.c

#### 1. init_paging()

 이 소스코드는 페이징을 위한 코드들이 있는 곳이다. init_paging()은 페이징을 하는데 필요한 변수들을 초기화하는 함수이다. 글쓴이가 이 소스코드에서 한 작업은 가상주소로 변환되는 작업들을 모두 수정하여 실 주소를 다루도록 바꾼 것이다. VH_TO_RH()를 사용한 부분들을 전부 사용하지 않도록 수정하였다. 
 
 이 함수가 하는 것은 페이지 디렉토리를 하나 만들고 페이지 디렉토리의 0번지에 페이지 테이블을 만든다. 그리고 NUM_PE번지 까지의 항목을 초기화 해주는데 글쓴이는 페이지 폴트가 일어나는 모습을 관찰하기 위해 NUM_PE를 259로 정했다.
 

#### 2. uint32_t pt_copy(uint32_t *pd, uint32_t *dest_pd, uint32_t idx, uint32_t *start, uint32_t *end, bool share)

 본 함수는 페이지 테이블을 복사하거나 공유시켜주는 함수이다. share 변수가 추가되면서, 공유기능과 복사기능을 구분하여 구현하였다. 공유기능은 pt_pde()를 이용하여 공유의 대상이 되는 페이지 테이블을 구하고 새롭게 페이지 테이블을 할당하여 그 페이지에 공유의 대상이 되는 페이지 테이블의 모든 엔트리의 주소를 저장함으로써 공유를 구현하였다. 복사의 경우 새로 만든 페이지 테이블에 새로운 페이지 프레임을 할당해주고, memcpy_hard()를 사용하여 기존 페이지 테이블 엔트리에 담겨진 페이지 프레임을 그대로 복사해줌으로써 복사기능을 구현하였다.
 

#### 3. uint32_t* pd_copy(uint32_t* from, uint32_t* to, uint32_t idx, uint32_t* start, uint32_t* end, bool share)

 이 함수는 page directory를 복사하는 함수이다. 추가된 start와 end인자는 사용되는 메모리의 시작과 끝 주소를 나타내는 인자이다. 
 
 페이지 디렉토리를 공유할 경우 현재 페이지 디렉토리 주소를 리턴하고 함수를 종료한다. page directory의 인덱스는 메모리의 주소에 따라 매핑된다. 따라서 탐색을 시작할 pde의 인덱스와 마지막 pde의 인덱스를 pde_idx_addr()을 사용하여 계산한 뒤 그 범위 내에서의 페이지 디렉토리 엔트리를 사용 중인지 확인한 후 pt_copy()로 넘겨서 복사를 진행한다.
 

#### 4. uint32_t * pd_create(pid_t pid)

 기존 함수에서 가상주소를 다루는 코드를 삭제하였다.
 

#### 5. void pf_handler(struct intr_frame *iframe)

 가상부분을 다루는 부분을 모두 삭제하였다.
 
 
### 2-2. 전체 구성도

![image](https://user-images.githubusercontent.com/40683361/118365589-50e9e000-b5d8-11eb-9025-f7679204384b.png)

다음은 SSUOS의 메모리관리 체계에 대한 구성도이다. SSUOS는 비트맵, 페이지 디렉토리, 페이지 테이블을 이용하여 페이지 프레임들을 관리한다. 그 과정은 다음과 같다. 페이지 할당요청을 하면 할당될 페이지가 페이지 테이블에 등록되어있는지 확인한다. 만약 그렇지 않다면 page fault가 발생하게 되고 pf_handler가 해당 주소를 페이지 테이블에 등록함으로써 page fault를 처리하게 된다. 그 후 페이지 프레임을 할당해주면서 해당 주소에 해당하는 비트맵의 영역을 set 해주면서 작업이 완료된다.
 
