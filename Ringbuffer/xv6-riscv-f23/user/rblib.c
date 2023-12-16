//suwei created the .c file
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "user/rblib.h"

void test(void)
{//suwei add the function
	fprintf(1,"test():%d\n",87);
}
void store(int *p, int v) {
  __atomic_store_8(p, v, __ATOMIC_SEQ_CST);
}
int load(int *p) {
  return __atomic_load_8(p, __ATOMIC_SEQ_CST);
}
void ringbuf_start_read(int ring_desc, char **addr, int *bytes)
{
	struct user_ring_buf urb;
	urb.buf=addr;
	urb.book=(urb.buf+(4096*16));
	//fprintf(1,"parent book: %d\n", (int)(load((int *)(urb.book)) ));
	//fprintf(1,"parent books: %d\n", (int)(load((int *)(urb.book+8)) ));
	while( (load((int *)(urb.book)) - load((int *)(urb.book+8)) != 65536) );//wait until write more than read;

	int nread=0;
	for(;;)
	{//read 4 byte at a loop
  		if(*(int*)(urb.buf+nread)!=13345435)
  			fprintf(1,"read error: %d\n",*(int*)(urb.buf+nread)); 
  		*(int*)(urb.buf+nread)=878787;//restore
		(*bytes)-=4;
		nread+=4;		
		if(*bytes<=0)
			break;
	}
	store((int *)(urb.book+8), (int)(*(int *)(urb.book+8)+nread));
}
void ringbuf_finish_read(int ring_desc, int bytes)
{
	//free map here
}
void ringbuf_start_write(int ring_desc, char **addr, int *bytes)
{
	struct user_ring_buf urb;
	urb.buf=addr;//assign addr to buf
	urb.book=(urb.buf+(4096*16));//assign addr to book
	while(  load((int *)(urb.book)) != load((int *)(urb.book+8)) );//wait until read catch up
	int nwrite =0;// *(uint64 *)(urb.book+8);
	for(;;)
	{//write 4 byte at a loop
		*(int*)(urb.buf+nwrite)=13345435;
		(*bytes)-=4;
		nwrite+=4;//add nread
		if(*bytes<=0)
			break;
	}
	store((int*)urb.book, *((int*)urb.book)+nwrite);	
	//fprintf(1,"child book: %d\n", (int)(load((int *)(urb.book)) ));
	//fprintf(1,"child books: %d\n", (int)(load((int *)(urb.book+8)) ));
}
void ringbuf_finish_write(int ring_desc, int bytes)
{
	//free map here
}

