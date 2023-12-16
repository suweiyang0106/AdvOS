//suwei added this file
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

#define MAX_RINGBUFS 10
#define RINGBUF_SIZE 16

struct ringbuf {
   int refcount; // 0 for empty slot
   char name[16];
   void *buf[RINGBUF_SIZE]; // physical addresses of pages that comprise the ring buffer
   void *book;
};
struct book {
   uint64 read_done, write_done;
};
struct spinlock ringbuf_lock;
struct ringbuf ringbufs[MAX_RINGBUFS];

#if 1
int suweimappage(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm, int opt)
{//this func maps va to new pa
  uint64 a, last;
  pte_t *pte;

  if(size == 0)
    panic("mappages: size");
  
  a = PGROUNDDOWN(va);
  last = PGROUNDDOWN(va + size - 1);
  for(;;){
    if((pte = walk(pagetable, a, 1)) == 0)
      return -1;
    if(*pte & PTE_V & opt)//suwei comment it because I need it.
    {  
    	//debuging...
    	//uint64 pa_old = PTE2PA(*pte);//suwei add
    	//kfree((void*)pa_old);//suwei add
    	//panic("mappages: remap");//suwei comment
    }
    *pte = PA2PTE(pa) | perm | PTE_V;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}
#else//suwei:work code, not easily change
int suweimappage(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm, int opt)
{//this func maps va to new pa
  uint64 a, last;
  pte_t *pte;

  if(size == 0)
    panic("mappages: size");
  
  a = PGROUNDDOWN(va);
  last = PGROUNDDOWN(va + size - 1);
  for(;;){
    if((pte = walk(pagetable, a, 1)) == 0)
      return -1;
    //if(*pte & PTE_V & opt)//suwei comment it because I need it.
    	//panic("mappages: remap");
    *pte = PA2PTE(pa) | perm | PTE_V;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}
#endif
int namecomp(const char *name,char*rb_name)
{//return 1: found 0: not found
	if(sizeof(name)==sizeof(rb_name))
	{
		for(int i=0;i<sizeof(name);i++)
		{
			if(name[i]!=rb_name[i])
				return 0;
		}
		return 1;
	}
	return 0;
}

int find_ringbuf(const char *name)
{
	for(int i=0;i<MAX_RINGBUFS;i++)
	{
		if(namecomp(name,ringbufs[i].name)==1)
			return i;
	}
	return -1;
}
int find_avalringbuf()
{
	for(int i=0;i<MAX_RINGBUFS;i++)
	{
		if(ringbufs[i].name[0]=='\000')//verify name is empted
			return i;
	}
	return -1;	
}
void name_rbf(const char *name, int idx)
{//name ringbufs[idx]
	for(int i=0;i<16;i++)
		ringbufs[idx].name[i]= name[i];//copy name from user
}

int map_rbf(void ** addr, int idx, int opt)
{//map va and pa, opt: 0:map new, 1: map existing
	struct proc *p = myproc();//used to map	
	pagetable_t pagetable = p->pagetable;
	uint64 *mem;//pa tmp variable
	uint64 va;
	if(opt==0)
	{//create new space for ringbuf
		int i=0;
		for(va = (uint64)addr; va < (uint64)addr+((RINGBUF_SIZE+1) * PGSIZE) ; va += PGSIZE)
		{	
		#if 1
			mem = kalloc();
			if(mem==0)
				return -1;
			if(i<16)
				ringbufs[idx].buf[i]=mem;
			else
				ringbufs[idx].book=mem;
			i++;
			memset(mem, 0x00, PGSIZE);
			suweimappage(pagetable, va, PGSIZE, (uint64)mem, PTE_R|PTE_U|PTE_W,1);
			
		#else//debugging...
			va = PGROUNDDOWN(va);
			uint64 pa;
			pa = walkaddr(pagetable,va);
			if(pa==0)
				return -1;
			memset((uint64*)pa, 0x00, PGSIZE);//test
			if(i<16)
				ringbufs[idx].buf[i]=(void *)pa;
			else
				ringbufs[idx].book=(void *)pa;
			i++;
		#endif
		}
		ringbufs[idx].refcount++;//add ref count
	}
	else
	{//map va and pa from ringbuf
		int i=0;
		for(va = (uint64)addr; va < (uint64)addr+((RINGBUF_SIZE+1) * PGSIZE) ; va += PGSIZE)
		{
			if(i<16)
				mem=ringbufs[idx].buf[i];
			else
				mem=ringbufs[idx].book;
			i++;
			if(suweimappage(pagetable, va, PGSIZE, (uint64)mem, PTE_R|PTE_U|PTE_W,0)==-1)
				return -1;
		}
		ringbufs[idx].refcount++;//add ref count
	}
	return 0;
}

int ringbufalloc(const char *name, int open/* 1 for open, 0 for close*/ ,void **addr)
{
	//JOBs:
	//1.find avaliable ringbufs (with name: skip it /without name: name it)
	//2.map va and pa, name it 
	//3.deallocate it if close
	int rbid = -1; 
	int opt=0;//0:create new space for ringbuf and map it 1:map existing ringbuf
	if(open ==2)
	{	//open 2:initial lock, 1:open, 0:close
		initlock(&ringbuf_lock, "rb");	
		return 0;
	}
	if(open==1)
	{//allocate ringbuf and map va/pa		
		acquire(&ringbuf_lock);	
		rbid = find_ringbuf(name);//get existing ringbuf
		if(rbid==-1)
		{
			opt = 0;
			rbid = find_avalringbuf();//get avaliable ringbuf
		}
		else
			opt =1;
		if(rbid==-1)
			return -1;//not found any rb
		if(ringbufs[rbid].refcount==2)
			return -1;//not over two owner for ringbuf
		name_rbf(name, rbid);//name the buf, not matter if it is existing
		if(map_rbf(addr,rbid,opt)==-1)//map va and pa
		{
			release(&ringbuf_lock);	
			return -1;	
		}
		release(&ringbuf_lock);	
	}
	else
	{//deallocate ringbuf
		rbid = find_ringbuf(name);
		if(rbid==-1)
			return rbid;
		//should do something here
	}
	return rbid;
	
	#if 0//get va and pa; map them
	struct proc *p = myproc();//used to map	
	pagetable_t pagetable = p->pagetable;
	uint64 va;//va above trampline
	char *mem;//pa tmp variable
	for(va = (uint64)addr; va < (uint64)addr+((RINGBUF_SIZE+1) * PGSIZE) ; va += PGSIZE)
	{
		mem = kalloc();
		if(mem==0)
			return -1;
		memset(mem, 87, PGSIZE);
		suweimappage(pagetable, va, PGSIZE, (uint64)mem, PTE_R|PTE_U|PTE_W);	
	}
	#endif
	
	#if 0 //initialize ringbuf
	int i=0;
	for(i=0;i<16;i++)
	{
		if((ringbufs[0].buf[i] = kalloc()) == 0)//assign 64K page space to buf
		{	
			kfree(ringbufs[0].buf[i]);
			return -1;
		}			
		ringbufs[0].name[i]= name[0];//copy name from user	
			
	}
	if((ringbufs[0].book = kalloc()) == 0)//assign a page sapce to book
	{
			kfree(ringbufs[0].book);
			return -1;		
	}
	//addr=ringbufs[0].buf[0];//map to user addr
	//*(int*)ringbufs[0].buf[0]=8787;//used to confirm buffer can be write after returning to user.	
	#endif
	
	//return va;
}
