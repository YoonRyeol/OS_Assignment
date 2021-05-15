#include <filesys/inode.h>
#include <proc/proc.h>
#include <device/console.h>
#include <mem/palloc.h>

//빈칸을 구현해야 하는 함수
int file_open(struct inode *inode, int flags, int mode)
{
	int fd;
	struct ssufile **file_cursor = cur_process->file;

	for(fd = 0; fd < NR_FILEDES; fd++)
	{
		if(file_cursor[fd] == NULL)
		{
			if( (file_cursor[fd] = (struct ssufile *)palloc_get_page()) == NULL)
				return -1;
			break;
		}	
	}

	
	inode->sn_refcount++;

	file_cursor[fd]->inode = inode;
	file_cursor[fd]->pos = 0;


	if(flags & O_APPEND){
	//O_APPEND 플래그 기능을 구현
		file_cursor[fd]->pos = inode->sn_size;
		//파일의 오프셋(pos)를 현재 파일의 사이즈로 바꿔주어 끝으로 이동시킨다.
		flags |= O_APPEND;
		//플래그 정보에 현재 플래그 추가
	}
	else if(flags & O_TRUNC){
	//O_TRUNC 플래그 기능을 구현
		flags |= O_TRUNC;
		//플래그 정보에 현태 플래그를 추가
		file_cursor[fd]->inode->sn_size = 0;
		//inode에 저장되는 파일의 크기를 0으로 만듬
	}

	file_cursor[fd]->flags = flags;
	file_cursor[fd]->unused = 0;
	return fd;
}

int file_write(struct inode *inode, size_t offset, void *buf, size_t len)
{
	return inode_write(inode, offset, buf, len);
}

int file_read(struct inode *inode, size_t offset, void *buf, size_t len)
{
	return inode_read(inode, offset, buf, len);
}
