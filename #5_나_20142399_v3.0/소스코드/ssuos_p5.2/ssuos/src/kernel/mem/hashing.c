#include <device/io.h>
#include <mem/mm.h>
#include <mem/paging.h>
#include <device/console.h>
#include <proc/proc.h>
#include <interrupt.h>
#include <mem/palloc.h>
#include <ssulib.h>
#include <mem/hashing.h>
#include <stdlib.h>

//인덱스 함수 자체가 hash()의 역할을 하고 있음
uint32_t F_IDX(uint32_t addr, uint32_t capacity) {
    return addr % ((capacity / 2) - 1);
}

uint32_t S_IDX(uint32_t addr, uint32_t capacity) {
    return (addr * 7) % ((capacity / 2) - 1) + capacity / 2;
}

//hash table을 초기화시키는 함수, token을 모두 0으로 초기화 한다.
void init_hash_table(void)
{
	// TODO : OS_P5 assignment
    
    for(int i=0; i<CAPACITY/2; i++){
        for(int j=0; j<4; j++){
            hash_table.bottom_buckets[i].token[j]=0;
            hash_table.bottom_buckets[i].num=0;
        }
    }

    for(int i=0; i<CAPACITY; i++){
        for(int j=0; j<4; j++){
            hash_table.top_buckets[i].token[j]=0;
            hash_table.bottom_buckets[i].num=0;
        }
    }

}

//hash table에 원소를 삽입하는 함수이다. addr은 페이지의 주소, capacity는 top bucket의 크기이다.
void insert_elem(uint32_t addr, uint32_t capacity){
    //해쉬 함수 두개를 이용하여 첫 번째 인덱스와 두 번째 인덱스를 만든다.
    uint32_t top_idx1 = F_IDX(addr, capacity);
    uint32_t top_idx2 = S_IDX(addr, capacity);
    //top bucket의 인덱스들을 이용하여 bottom bucket의 인덱스를 만든다.
    uint32_t bot_idx1 = top_idx1/2;
    uint32_t bot_idx2 = top_idx2/2;
    int cur_slot = 0;

    //top bucket부터 원소를 삽입한다. 슬롯을 오름차순으로 빈 자리가 있는지 확인하며 이는 첫 번째 bucket, 두 번째 bucket
    //각각 번갈아 가면서 시행된다. 빈자리가 있을경우 원소를 삽입하고 해당 버킷의 num변수를 1 더함으로써 현재 원소수를 나타낸다.
    for(int i=0; i<4; i++){
        if(hash_table.top_buckets[top_idx1].token[i] != 1){
            hash_table.top_buckets[top_idx1].num++;
            hash_table.top_buckets[top_idx1].token[i] = 1;
            hash_table.top_buckets[top_idx1].slot[i].key = pte_idx_addr(addr);
            hash_table.top_buckets[top_idx1].slot[i].value = VH_TO_RH(addr);
            printk("hash value inserted in top level : idx : %3d, key : %2d, value : %x\n", top_idx1, pte_idx_addr(addr), VH_TO_RH(addr));
            return;
        }
        if(hash_table.top_buckets[top_idx2].token[i] != 1){
            hash_table.top_buckets[top_idx2].num++;
            hash_table.top_buckets[top_idx2].token[i] = 1;
            hash_table.top_buckets[top_idx2].slot[i].key = pte_idx_addr(addr);
            hash_table.top_buckets[top_idx2].slot[i].value = VH_TO_RH(addr);
            printk("hash value inserted in top level : idx : %3d, key : %2d, value : %x\n", top_idx2, pte_idx_addr(addr), VH_TO_RH(addr));
            return;
        }
    }

    //bottom bucket에 원소를 삽입한다. top bucket에 원소를 삽입할 때와 같이 삽입한다.
    for(int i=0; i<4; i++){
        if(hash_table.bottom_buckets[bot_idx1].token[i] != 1){
            hash_table.bottom_buckets[bot_idx1].num++;
            hash_table.bottom_buckets[bot_idx1].token[i] = 1;
            hash_table.bottom_buckets[bot_idx1].slot[i].key = pte_idx_addr(addr);
            hash_table.bottom_buckets[bot_idx1].slot[i].value = VH_TO_RH(addr);
            printk("hash value inserted in bottom level : idx : %3d, key : %2d, value : %x\n", bot_idx1, pte_idx_addr(addr), VH_TO_RH(addr));
            return;
        }
        if(hash_table.bottom_buckets[bot_idx2].token[i] != 1){
            hash_table.bottom_buckets[bot_idx2].num++;
            hash_table.bottom_buckets[bot_idx2].token[i] = 1;
            hash_table.bottom_buckets[bot_idx2].slot[i].key = pte_idx_addr(addr);
            hash_table.bottom_buckets[bot_idx2].slot[i].value = VH_TO_RH(addr);
            printk("hash value inserted in bottom level : idx : %3d, key : %2d, value : %x\n", bot_idx2, pte_idx_addr(addr), VH_TO_RH(addr));
            return;
        }
    }

    //이제 대응되는 자리가 아무 곳에도 없을 때
    //1. 첫 번째 top level 인덱스의 bucket의 원소들을 옮길 수 있는지 먼저 확인한다. 슬롯을 오름차순으로 확인한다. 
    //옮길 수 있다면 원소이동을 시키고 빈 자리에 현재 삽입하려는 원소를 삽입한다. 
    uint32_t *moved_real_address;
    uint32_t first_idx;
    uint32_t second_idx;
    for(int i=0; i<4; i++){
        moved_real_address = RH_TO_VH(hash_table.top_buckets[top_idx1].slot[i].value);
        first_idx = F_IDX(moved_real_address,capacity);
        second_idx = S_IDX(moved_real_address,capacity);
        if(first_idx != top_idx1 && hash_table.top_buckets[first_idx].num != 4){
        //첫 번째 인덱스가 원소이동의 대상이 되는 원소의 현재 인덱스가 아니며, 첫 번째 인덱스의 bucket에 빈 자리가 있을 경우
            for(int j=0; j<4; j++){
                if(hash_table.top_buckets[first_idx].token[j] == 0){
                    //해당 bucket에서 빈자리를 찾으면 먼저 원소이동을 한다.
                    hash_table.top_buckets[first_idx].num++;
                    hash_table.top_buckets[first_idx].token[j] = 1;
                    hash_table.top_buckets[first_idx].slot[j].key = hash_table.top_buckets[top_idx1].slot[i].key;
                    hash_table.top_buckets[first_idx].slot[j].value = hash_table.top_buckets[top_idx1].slot[i].value;
                    //현재 삽입하려는 원소를 빈 자리에 넣어준다.
                    printk("hash value inserted in top level : idx : %3d, key : %2d, value : %x\n", top_idx1, pte_idx_addr(addr), VH_TO_RH(addr));
                    hash_table.top_buckets[top_idx1].slot[i].key = pte_idx_addr(addr);
                    hash_table.top_buckets[top_idx1].slot[i].value = VH_TO_RH(addr);
                    return;
                }
            }
        }
        else if (second_idx != top_idx1 && hash_table.top_buckets[second_idx].num != 4){
        //두 번째 인덱스가 원소이동의 대상이 되는 원소의 현재 인덱스가 아니며, 두 번째 인덱스의 bucket에 빈 자리가 있을 경우
            for(int j=0; j<4; j++){
                if(hash_table.top_buckets[second_idx].token[j] == 0){
                    //해당 버킷에서 빈자리를 찾으면 먼저 원소이동을 한다.
                    hash_table.top_buckets[second_idx].num++;
                    hash_table.top_buckets[second_idx].token[j] = 1;
                    hash_table.top_buckets[second_idx].slot[j].key = hash_table.top_buckets[top_idx1].slot[i].key;
                    hash_table.top_buckets[second_idx].slot[j].value = hash_table.top_buckets[top_idx1].slot[i].value;
                    //현재 삽입하려는 원소를 빈 자리에 넣어준다.
                    printk("hash value inserted in top level : idx : %3d, key : %2d, value : %x\n", top_idx1, pte_idx_addr(addr), VH_TO_RH(addr));
                    hash_table.top_buckets[top_idx1].slot[i].key = pte_idx_addr(addr);
                    hash_table.top_buckets[top_idx1].slot[i].value = VH_TO_RH(addr);
                    return;
                }
            }
        }
    }
    //원소 이동 및 원소 삽입의 세부적인 구현은 이하 동일 하다.


    //2. 두 번째 top level 인덱스에 대해 원소이동을 시도한다.
    for(int i=0; i<4; i++){
        moved_real_address = RH_TO_VH(hash_table.top_buckets[top_idx2].slot[i].value);
        first_idx = F_IDX(moved_real_address,capacity);
        second_idx = S_IDX(moved_real_address,capacity);
        if(first_idx != top_idx2 && hash_table.top_buckets[first_idx].num != 4){
            for(int j=0; j<4; j++){
                if(hash_table.top_buckets[first_idx].token[j] == 0){
                    hash_table.top_buckets[first_idx].num++;
                    hash_table.top_buckets[first_idx].token[j] = 1;
                    hash_table.top_buckets[first_idx].slot[j].key = hash_table.top_buckets[top_idx2].slot[i].key;
                    hash_table.top_buckets[first_idx].slot[j].value = hash_table.top_buckets[top_idx2].slot[i].value;
                    printk("hash value inserted in top level : idx : %3d, key : %2d, value : %x\n", top_idx2, pte_idx_addr(addr), VH_TO_RH(addr));
                    hash_table.top_buckets[top_idx2].slot[i].key = pte_idx_addr(addr);
                    hash_table.top_buckets[top_idx2].slot[i].value = VH_TO_RH(addr);
                    return;
                }
            }
        }
        else if (second_idx != top_idx2 && hash_table.top_buckets[second_idx].num != 4){
            for(int j=0; j<4; j++){
                if(hash_table.top_buckets[second_idx].token[j] == 0){
                    hash_table.top_buckets[second_idx].num++;
                    hash_table.top_buckets[second_idx].token[j] = 1;
                    hash_table.top_buckets[second_idx].slot[j].key = hash_table.top_buckets[top_idx2].slot[i].key;
                    hash_table.top_buckets[second_idx].slot[j].value = hash_table.top_buckets[top_idx2].slot[i].value;
                    printk("hash value inserted in top level : idx : %3d, key : %2d, value : %x\n", top_idx2, pte_idx_addr(addr), VH_TO_RH(addr));
                    hash_table.top_buckets[top_idx2].slot[i].key = pte_idx_addr(addr);
                    hash_table.top_buckets[top_idx2].slot[i].value = VH_TO_RH(addr);
                    return;
                }
            }
        }
    }
    //3. 첫 번째 bottom level 인덱스에 대해 원소이동을 시도한다.
    for(int i=0; i<4; i++){
        moved_real_address = RH_TO_VH(hash_table.bottom_buckets[bot_idx1].slot[i].value);
        first_idx = F_IDX(moved_real_address,capacity);
        second_idx = S_IDX(moved_real_address,capacity);
        first_idx /= 2;
        second_idx /= 2;
        if(first_idx != bot_idx1 && hash_table.bottom_buckets[first_idx].num != 4){
            for(int j=0; j<4; j++){
                if(hash_table.bottom_buckets[first_idx].token[j] == 0){
                    hash_table.bottom_buckets[first_idx].num++;
                    hash_table.bottom_buckets[first_idx].token[j] = 1;
                    hash_table.bottom_buckets[first_idx].slot[j].key = hash_table.bottom_buckets[bot_idx1].slot[i].key;
                    hash_table.bottom_buckets[first_idx].slot[j].value = hash_table.bottom_buckets[bot_idx1].slot[i].value;
                    printk("hash value inserted in bottom level: idx : %3d, key : %2d, value : %x\n", bot_idx1, pte_idx_addr(addr), VH_TO_RH(addr));
                    hash_table.bottom_buckets[bot_idx1].slot[i].key = pte_idx_addr(addr);
                    hash_table.bottom_buckets[bot_idx1].slot[i].value = VH_TO_RH(addr);
                    return;
                }
            }
        }
        else if (second_idx != bot_idx1 && hash_table.bottom_buckets[second_idx].num != 4){
            for(int j=0; j<4; j++){
                if(hash_table.bottom_buckets[second_idx].token[j] == 0){
                    hash_table.bottom_buckets[second_idx].num++;
                    hash_table.bottom_buckets[second_idx].token[j] = 1;
                    hash_table.bottom_buckets[second_idx].slot[j].key = hash_table.bottom_buckets[bot_idx1].slot[i].key;
                    hash_table.bottom_buckets[second_idx].slot[j].value = hash_table.bottom_buckets[bot_idx1].slot[i].value;
                    printk("hash value inserted in bottom level : idx : %3d, key : %2d, value : %x\n", bot_idx1, pte_idx_addr(addr), VH_TO_RH(addr));
                    hash_table.bottom_buckets[bot_idx1].slot[i].key = pte_idx_addr(addr);
                    hash_table.bottom_buckets[bot_idx1].slot[i].value = VH_TO_RH(addr);
                    return;
                }
            }
        }
    }
//  4. 두 번째 bottom level 인덱스에 대해 원소이동을 시도한다.
    for(int i=0; i<4; i++){
        moved_real_address = RH_TO_VH(hash_table.bottom_buckets[bot_idx2].slot[i].value);
        first_idx = F_IDX(moved_real_address,capacity);
        second_idx = S_IDX(moved_real_address,capacity);
        first_idx /= 2;
        second_idx /= 2;
        if(first_idx != bot_idx2 && hash_table.bottom_buckets[first_idx].num != 4){
            for(int j=0; j<4; j++){
                if(hash_table.bottom_buckets[first_idx].token[j] == 0){
                    hash_table.bottom_buckets[first_idx].num++;
                    hash_table.bottom_buckets[first_idx].token[j] = 1;
                    hash_table.bottom_buckets[first_idx].slot[j].key = hash_table.bottom_buckets[bot_idx2].slot[i].key;
                    hash_table.bottom_buckets[first_idx].slot[j].value = hash_table.bottom_buckets[bot_idx2].slot[i].value;
                    printk("hash value inserted in bottom level : idx : %3d, key : %2d, value : %x\n", bot_idx2, pte_idx_addr(addr), VH_TO_RH(addr));
                    hash_table.bottom_buckets[bot_idx2].slot[i].key = pte_idx_addr(addr);
                    hash_table.bottom_buckets[bot_idx2].slot[i].value = VH_TO_RH(addr);
                    return;
                }
            }
        }
        else if (second_idx != bot_idx2 && hash_table.bottom_buckets[second_idx].num != 4){
            for(int j=0; j<4; j++){
                if(hash_table.bottom_buckets[second_idx].token[j] == 0){
                    hash_table.bottom_buckets[second_idx].num++;
                    hash_table.bottom_buckets[second_idx].token[j] = 1;
                    hash_table.bottom_buckets[second_idx].slot[j].key = hash_table.bottom_buckets[bot_idx2].slot[i].key;
                    hash_table.bottom_buckets[second_idx].slot[j].value = hash_table.bottom_buckets[bot_idx2].slot[i].value;
                    printk("hash value inserted in bottom level : idx : %3d, key : %2d, value : %x\n", bot_idx2, pte_idx_addr(addr), VH_TO_RH(addr));
                    hash_table.bottom_buckets[bot_idx2].slot[i].key = pte_idx_addr(addr);
                    hash_table.bottom_buckets[bot_idx2].slot[i].value = VH_TO_RH(addr);
                    return;
                }
            }
        }
    }
}

//addr에 해당하는 페이지의 주소를 가지는 원소를 제거한다.
void delete_elem(uint32_t addr, uint32_t capacity){
    
    //top level의 첫 번째, 두 번째 인덱스를 얻는다.
    uint32_t top_idx1 = F_IDX(addr, capacity);
    uint32_t top_idx2 = S_IDX(addr, capacity);
    //bottom level의 첫 번째, 두 번째 인덱스를 얻는다.
    uint32_t bot_idx1 = top_idx1/2;
    uint32_t bot_idx2 = top_idx2/2;
    //삭제하고자 하는 원소의 key, value를 얻는다.
    uint32_t target_key = pte_idx_addr(addr);
    uint32_t *target_value = VH_TO_RH(addr);

  
    //삭제하고자 하는 원소가 들어갈 수 있는 4개의 버킷을 오름차순으로 슬롯을 각각 확인한다. key, value 값이 같고 token이 1이면 
    //token을 0으로 바꿈으로써 해당 원소를 삭제한다. 해당 원소를 삭제하고 num--함으로써 현재 버킷의 원소 개수를 갱신한다.
    for(int i=0; i<4; i++){
        //top level의 첫 번째 인덱스에서 원소를 확인
        if(hash_table.top_buckets[top_idx1].slot[i].key == target_key && hash_table.top_buckets[top_idx1].slot[i].value == target_value
        && hash_table.top_buckets[top_idx1].token[i] == 1){
            printk("hash value detected : idx : %3d, key : %2d, value : %x\n", top_idx1, pte_idx_addr(addr), VH_TO_RH(addr));
            hash_table.top_buckets[top_idx1].token[i] = 0;
            hash_table.top_buckets[top_idx1].num--;
            return;
        }
        //top level의 두 번째 인덱스에서 원소를 확인
        else if(hash_table.top_buckets[top_idx2].slot[i].key == target_key && hash_table.top_buckets[top_idx2].slot[i].value == target_value
        && hash_table.top_buckets[top_idx2].token[i] == 1){
            printk("hash value detected : idx : %3d, key : %2d, value : %x\n", top_idx2, pte_idx_addr(addr), VH_TO_RH(addr));
            hash_table.top_buckets[top_idx2].num--;
            hash_table.top_buckets[top_idx2].token[i] = 0;
            return;
        }
        //bottom level의 첫 번째 인덱스에서 원소를 확인
        else if(hash_table.bottom_buckets[bot_idx1].slot[i].key == target_key && hash_table.bottom_buckets[bot_idx1].slot[i].value == target_value
        && hash_table.bottom_buckets[bot_idx1].token[i] == 1){
            printk("hash value detected : idx : %3d, key : %2d, value : %x\n", bot_idx1, pte_idx_addr(addr), VH_TO_RH(addr));
            hash_table.bottom_buckets[bot_idx1].token[i] = 0;
            hash_table.bottom_buckets[bot_idx1].num--;
            return;
        }
        //bottom level 두 번째 인덱스에서 원소를 확인
        else if(hash_table.bottom_buckets[bot_idx2].slot[i].key == target_key && hash_table.bottom_buckets[bot_idx2].slot[i].value == target_value
        && hash_table.bottom_buckets[bot_idx2].token[i] == 1){
            printk("hash value detected : idx : %3d, key : %2d, value : %x\n", bot_idx2, pte_idx_addr(addr), VH_TO_RH(addr));
            hash_table.bottom_buckets[bot_idx2].token[i] = 0;
            hash_table.bottom_buckets[bot_idx2].num--;
            return;
        }
    }

 
    //임의로 다른 곳에 삽입된 원소들을 제거한다.
    //top level에서 원소를 확인
    for(int i=0; i<CAPACITY; i++){
        for(int j=0; j<4; j++)
            if(hash_table.top_buckets[i].slot[j].key == target_key && hash_table.top_buckets[i].slot[j].value == target_value
            && hash_table.top_buckets[i].token[j] == 1){
                printk("hash value detected : idx : %3d, key : %2d, value : %x\n", i, pte_idx_addr(addr), VH_TO_RH(addr));
                hash_table.top_buckets[i].token[j] = 0;
                hash_table.top_buckets[i].num--;
                return;
            }
    }
    //bottom level에서 원소를 확인
    for(int i=0; i<CAPACITY/2; i++){
        for(int j=0; j<4; j++)
            if(hash_table.bottom_buckets[i].slot[j].key == target_key  && hash_table.bottom_buckets[i].slot[j].value == target_value
            && hash_table.bottom_buckets[i].token[j] == 1){
                printk("hash value detected : idx : %3d, key : %2d, value : %x\n", i, pte_idx_addr(addr), VH_TO_RH(addr));
                hash_table.bottom_buckets[i].token[j] = 0;
                hash_table.bottom_buckets[i].num--;
                return;
            }
    }


}
