#include <proc/sched.h>
#include <proc/proc.h>
#include <device/device.h>
#include <interrupt.h>
#include <device/kbd.h>
#include <filesys/file.h>

pid_t do_fork(proc_func func, void* aux1, void* aux2)
{
	pid_t pid;
	struct proc_option opt;

	opt.priority = cur_process-> priority;
	pid = proc_create(func, &opt, aux1, aux2);

	return pid;
}

void do_exit(int status)
{
	cur_process->exit_status = status; 	//종료 상태 저장
	proc_free();						//프로세스 자원 해제
	do_sched_on_return();				//인터럽트 종료시 스케줄링
}

pid_t do_wait(int *status)
{
	while(cur_process->child_pid != -1)
		schedule();
	//SSUMUST : schedule 제거.

	int pid = cur_process->child_pid;
	cur_process->child_pid = -1;

	extern struct process procs[];
	procs[pid].state = PROC_UNUSED;

	if(!status)
		*status = procs[pid].exit_status;

	return pid;
}

void do_shutdown(void)
{
	dev_shutdown();
	return;
}

int do_ssuread(void)
{
	return kbd_read_char();
}

int do_open(const char *pathname, int flags)
{
	struct inode *inode;
	struct ssufile **file_cursor = cur_process->file;
	int fd;

	for(fd = 0; fd < NR_FILEDES; fd++)
		if(file_cursor[fd] == NULL) break;

	if (fd == NR_FILEDES)
		return -1;

	if ( (inode = inode_open(pathname, flags)) == NULL)
		return -1;
	
	if (inode->sn_type == SSU_TYPE_DIR)
		return -1;

	fd = file_open(inode,flags,0);
	
	return fd;
}

int do_read(int fd, char *buf, int len)
{
	return generic_read(fd, (void *)buf, len);
}
int do_write(int fd, const char *buf, int len)
{
	return generic_write(fd, (void *)buf, len);
}

int do_fcntl(int fd, int cmd, long arg)
{
	int flag = -1;
	struct ssufile **file_cursor = cur_process->file;

//F_DUPFD flag 구현,
//먼저 arg로 받은 파일 디스크립터의 위치에 복사를 시도
//이미 할당이 되어있다면 반복문을 이용하여 arg로부터 이동
//빈 자리가 있을 때까지 이동하여 빈 자리가 있을 경우 
//해당 파일 디스크립터가 현재 파일 디스크립터가 가리키는
//파일 구조체를 가리키도록 구현함.
	if (cmd & F_DUPFD){
		if(file_cursor[arg] != NULL){
		//arg로 받은 자리가 이미 할당되어 있음
			for(; arg < NR_FILEDES; arg++)
		//빈 자리를 찾을 때 까지 순회
				if(file_cursor[arg] == NULL){
					//빈 자리가 있음
					file_cursor[arg] = file_cursor[fd];
					break;
			}
		}
		else
		//arg로 받은 자리가 비어있음
			file_cursor[arg] = file_cursor[fd];
		return arg;	
	}
//F_GETFL 플래그를 구현
//파일 구조체에서 플래그 정보를 담고있는 flags변수를 참조하여
//flag변수에 값을 저장하고 리턴
	else if (cmd & F_GETFL){
		flag = file_cursor[fd]->flags;
		return flag;
	}
//F_SETFL 플래그를 구현
//파일 구조체에서 플래그 정보를 담고있는 flags변수에 인자로 받은
//플래그의 값을 저장하고 새로운 플래그를 가지고 새롭게 inode를 만든다.
//새롭게 만들어진 inode는 플래그의 변경된 값이 반영되어있다.
//기존의 파일 디스크립터가 새로운 inode를 참조하도록 한다.
//최종적으로 플래그의 상태가 반영된 파일구조체가 생성된다.
	else if(cmd & F_SETFL){
		//플래그 확인, 파일을 열때 기존의 플래그에 O_APPEND를 추가한 경우, 또는 인자가 O_APPEND일때가 아닌 경우에는 -1 리턴
		if(!((arg == O_APPEND) || ((arg & O_APPEND) && (arg & file_cursor[fd]->flags))))
			return -1;
		int new_fd;
		file_cursor[fd]->flags |= arg;
		//플래그 정보를 저장
		new_fd = file_open(file_cursor[fd]->inode, file_cursor[fd]->flags, 0);
		//새로운 inode를 생성
		palloc_free_page(file_cursor[fd]);
		//기존의 inode를 free해줌
		file_cursor[fd] = file_cursor[new_fd];
		//새로운 inode를 기존의 fd가 가리키도록 함
		file_cursor[new_fd] = NULL;
		return file_cursor[fd]->flags;
		
	}
	else{
		return -1;
	}

}
