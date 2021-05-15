#include <list.h>
#include <proc/sched.h>
#include <mem/malloc.h>
#include <proc/proc.h>
#include <proc/switch.h>
#include <interrupt.h>

extern struct list plist;
extern struct list rlist;
extern struct list runq[RQ_NQS];

extern struct process procs[PROC_NUM_MAX];
extern struct process *idle_process;
struct process *latest;

bool more_prio(const struct list_elem *a, const struct list_elem *b,void *aux);
int scheduling; 					// interrupt.c
bool timerOn = true;


struct process* get_next_proc(void) 
{
	bool found = false;
	struct process *next = NULL;
	struct list_elem *elem;

	/* 
	   You shoud modify this function...
	   Browse the 'runq' array 
	*/

//	현재 프로세스가 0번 프로세스가 아닌 경우
//	명세에 따라 0번 프로세스의 주소를 리턴한다.
	if(cur_process->pid != 0)
		return &procs[0];
//	현재 프로세스가 0번 프로세스일 경우
//	가장 우선순위가 높은 리스트의 맨 앞의 프로세스의 주소를 리턴한다. 
	else{
		for(int i=0; i<RQ_NQS; i++){
			for(elem = list_begin(&runq[i]); elem != list_end(&runq[i]); elem = list_next(elem)){
				struct process *p = list_entry(elem, struct process, elem_stat);
				return p;
			}
		}
	}
}


void schedule(void)
{
	/* You shoud modify this function.... */
	struct process *cur;
	struct process *next;
	struct list_elem *elem;
//	context switching 동안 tick이 증가하지 않도록 하기 위해
//	lock을 설정한다.
	cur_process->simple_lock = 1;
//	interrupt 실행을 불가능하게 설정한다.
	intr_disable();
	proc_wake();


//runq 의 모든 멤버를 pid 순으로 출력한다.
	bool isFirst = true;
	for(int target_pid=1; target_pid<100 && cur_process->pid == 0; target_pid++){
		for(int i=0; i<RQ_NQS; i++){
			for(elem = list_begin(&runq[i]); elem != list_end(&runq[i]); elem=list_next(elem)){
				struct process *p = list_entry(elem, struct process, elem_stat);
				if(p->pid == target_pid){
					if(isFirst){
						//printk("#=%4d p=%4d c=%4d u=%4d", p->pid, p->priority, p->time_slice, p->time_used);
						printk("#=%4d p=%4d ", p->pid, p->priority);
						printk("c=%4d u=%4d", p->time_slice, p->time_used);
						isFirst = false;
					}
					else
					{
						printk(", #=%4d p=%4d ", p->pid, p->priority);
						printk("c=%4d u=%4d",p->time_slice, p->time_used);
					}
				}
			}
		}
	}


	next = get_next_proc();
	if(next->pid != 0) // 다음 프로세스의 pid가 0이 아닐 때에만 출력
		printk("\nSelected # = %1d\n", next->pid);
	cur = cur_process;
	cur_process = next;
	cur_process->time_slice = 0;
	cur->simple_lock = 0;
//	context switching을 수행한다.
	switch_process(cur, next);
//	interrupt 실행을 가능하게 설정한다.
	intr_enable();	
 }
