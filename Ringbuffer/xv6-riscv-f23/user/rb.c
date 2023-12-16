//Result:
//original pipe: 0.03MB/tick (10MB per 337 ticks)
//modified pipe:0.15MB/tick  (10MB per 66 ticks)
//ring buffer: 14.49MB/tick  (1000MB per 69 ticks)

#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "user.h"
#include "rblib.h"
char ringbf[4096*16 + 4096];//buf(64k)+book(4k)
struct user_ring_buf urb;
void main(void)
{
  int start_tick = uptime();
  ringbuf("initial lock", 2, (void**)ringbf);//only inital ring buf lock

  if(fork() == 0)
  {  	
  	ringbuf("writeit", 1, (void**)ringbf);//create a ring buf 
  	urb.buf=ringbf;
  	urb.book=(ringbf+(4096*16));  	
  	//fprintf(1,"child writing...\n",20);
  	for(int i = 0;i < 16000; i++)
  	{
  		//write 10MB overall
		int wr_byte=64*1024;//64KB
		ringbuf_start_write(-1, (char**)ringbf, &wr_byte);//write 64k at once
		//*(int*)(urb.buf)=9487;
		//*(int*)(urb.book)=74;
	}
	//fprintf(1,"child write done\n",20);
	//ringbuf("writeit", 0, (void**)ringbf);//demap a ring buf
    exit(0);
    fprintf(1,"Child somehow fail\n",20);  
  }
  else
  {  	
  	
  	struct user_ring_buf urb;  	
  	ringbuf("writeit", 1, (void**)ringbf);//create a ring buf	
  	urb.buf=ringbf;
  	urb.book=(ringbf+(4096*16));  		
  	for(int i = 0;i < 16000; i++)
  	{
  		//read 10 MB overall
		int rd_byte=64*1024;//64KB
	  	ringbuf_start_read(-1,(char**)ringbf,&rd_byte);//read 64k at once	  	
  	}   			   		
  	fprintf(1,"parent read done\n",20);
  	fprintf(1,"parent read input[0] %d\n",*(int*)(urb.buf));
  	fprintf(1,"parent read %d bytes\n",*(int*)urb.book);
  	fprintf(1,"child write %d bytes\n",*(int*)(urb.book+8));
  }
  wait(0); 
  fprintf(1,"Finished, all correct dvb \nParent elapsed ticks: %d\n",uptime()-start_tick);
  //ringbuf("writeit", 0, (void**)ringbf);//demap a ring buf
}
  
