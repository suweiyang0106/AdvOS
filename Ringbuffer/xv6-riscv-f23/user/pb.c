#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "user.h"
    
 //passing command line arguments
int data_compare(char*input, char*buf)
{
	for(int i=0;i<sizeof(input);i++)
	{
		if(input[i]!=buf[i])
		{
			fprintf(1,"DMC data:\n",20);
			fprintf(1,"input:\n",20);
			fprintf(1,input,sizeof(input));
			fprintf(1,"read:\n",20);
			fprintf(1,buf,sizeof(buf));
			return -1;
		}	
	}
	return 0;
}
void main(void)
{
  //fprintf(1,"My first xv6 program\n");
  int p[2];
  char buf[512];
  char input[512]="1222345678910\n";
  int send_amount=10*1024*1024/sizeof(buf);
  int start_tick = uptime();
  fprintf(1,"Input size(each time): %d bytes\n",sizeof(input));
  pipe(p);
  if(fork() == 0)
  {//child    
    close(1);
    dup(p[1]);
    for(int i=0;i<send_amount;i++)
    {
    	//sending data from input to pipe buffer 16bytes each time
    	if(write(p[1], input, sizeof(input))==-1)
    	{
    		fprintf(1,"Write fail\n",20);
    		exit(0);
    	}
    }
    close(p[0]);
    exit(0);
    fprintf(1,"Child somehow fail\n",20);
  } 
  else 
  {//parent   
    close(p[1]);
    fprintf(1,"Parent is receiving message...\n",20);
    for(int i=0;i<send_amount;i++)
    {
    	//receiving data from pipe buffer and put into buf
		if(read(p[0], buf, sizeof(buf))==-1)
		{	
			fprintf(1,"Read fail\n",20);
			exit(0);
		}
		#if 1
		//comparing data for each receiving
		if(data_compare(input,buf)==-1)
		{
			fprintf(1,"DMC fail at loop:%d\n",i);
			exit(0);
		}	
		#endif
		buf[0]='\0';//clear buf
    }
    wait(0);
    close(p[1]);    
  }
  fprintf(1,"Finished, all correct dvb \nParent elapsed ticks: %d\n",uptime()-start_tick);

}
  
