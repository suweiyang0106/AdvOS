#include "kernel/types.h"
#include "kernel/net.h"
#include "kernel/stat.h"
#include "user/user.h"
#define improvedflag 0//suwei add
//
// send a UDP packet to the localhost (outside of qemu),
// and receive a response.
//
#define PGROUNDUP(sz)  (((sz)+4096-1) & ~(4096-1))//suwei add
#if improvedflag
static void
ping(uint16 sport, uint16 dport, int attempts, char* suweibuf)
#else
static void
ping(uint16 sport, uint16 dport, int attempts)
#endif
{
  int fd;
  #if improvedflag//suwei add
  //char *suweibuf=malloc(4096*2);//suwei: 2 pages, one for msg, one for mbuf  
  char *suweibuf_m = (char *)PGROUNDUP((uint64)(suweibuf));
  char *suweibuf_buf = suweibuf_m+20;//suwei add, leave 20 bytes for head, next, len 
  char *suweibuf_msg = suweibuf_buf+128;//suwei add, leaves 128 bytes for head
  char *msg = "cpy msg bla bla bla!";
  strcpy(suweibuf_msg,msg);
  #else
  char *msg = "a message from xv6!";
  char *obuf = malloc(strlen(msg));//suwei add
  strcpy(obuf,msg);//suwei add
  #endif
  uint32 dst;

  // 10.0.2.2, which qemu remaps to the external host,
  // i.e. the machine you're running qemu on.
  dst = (10 << 24) | (0 << 16) | (2 << 8) | (2 << 0);

  // you can send a UDP packet to any Internet address
  // by using a different dst.
  if((fd = connect(dst, sport, dport)) < 0){
    fprintf(2, "ping: connect() failed\n");
    exit(1);
  }
  
  for(int i = 0; i < attempts; i++) {
    #if improvedflag//suwei add
    if(write(fd, suweibuf, strlen(msg)) < 0){//suwei add
    #else    
    if(write(fd, obuf, strlen(obuf)) < 0){//suwei comment
    #endif
      fprintf(2, "ping: send() failed\n");
      exit(1);
    }
  }
  //printf("2 suweibuf sizeof(after map): %d, addr:%d, %s\n",sizeof(suweibuf_msg), (char*)suweibuf_msg,(char*)suweibuf_msg);//suwei add
  #if 0//suwei add
  char ibuf[128];
  int cc = read(fd, ibuf, sizeof(ibuf)-1);
  if(cc < 0){
    fprintf(2, "ping: recv() failed\n");
    exit(1);
  }
  #endif
  close(fd);

  #if 0//suwei add
  ibuf[cc] = '\0';
  if(strcmp(ibuf, "this is the host!") != 0){
    fprintf(2, "ping didn't receive correct payload\n");
    exit(1);
  }
  #endif  
}

// Encode a DNS name
static void
encode_qname(char *qn, char *host)
{
  char *l = host; 
  
  for(char *c = host; c < host+strlen(host)+1; c++) {
    if(*c == '.') {
      *qn++ = (char) (c-l);
      for(char *d = l; d < c; d++) {
        *qn++ = *d;
      }
      l = c+1; // skip .
    }
  }
  *qn = '\0';
}

// Decode a DNS name
static void
decode_qname(char *qn, int max)
{
  char *qnMax = qn + max;
  while(1){
    if(qn >= qnMax){
      printf("invalid DNS reply\n");
      exit(1);
    }
    int l = *qn;
    if(l == 0)
      break;
    for(int i = 0; i < l; i++) {
      *qn = *(qn+1);
      qn++;
    }
    *qn++ = '.';
  }
}

// Make a DNS request
static int
dns_req(uint8 *obuf)
{
  int len = 0;
  
  struct dns *hdr = (struct dns *) obuf;
  hdr->id = htons(6828);
  hdr->rd = 1;
  hdr->qdcount = htons(1);
  
  len += sizeof(struct dns);
  
  // qname part of question
  char *qname = (char *) (obuf + sizeof(struct dns));
  char *s = "pdos.csail.mit.edu.";
  encode_qname(qname, s);
  len += strlen(qname) + 1;

  // constants part of question
  struct dns_question *h = (struct dns_question *) (qname+strlen(qname)+1);
  h->qtype = htons(0x1);
  h->qclass = htons(0x1);

  len += sizeof(struct dns_question);
  return len;
}

// Process DNS response
static void
dns_rep(uint8 *ibuf, int cc)
{
  struct dns *hdr = (struct dns *) ibuf;
  int len;
  char *qname = 0;
  int record = 0;

  if(cc < sizeof(struct dns)){
    printf("DNS reply too short\n");
    exit(1);
  }

  if(!hdr->qr) {
    printf("Not a DNS reply for %d\n", ntohs(hdr->id));
    exit(1);
  }

  if(hdr->id != htons(6828)){
    printf("DNS wrong id: %d\n", ntohs(hdr->id));
    exit(1);
  }
  
  if(hdr->rcode != 0) {
    printf("DNS rcode error: %x\n", hdr->rcode);
    exit(1);
  }
  
  //printf("qdcount: %x\n", ntohs(hdr->qdcount));
  //printf("ancount: %x\n", ntohs(hdr->ancount));
  //printf("nscount: %x\n", ntohs(hdr->nscount));
  //printf("arcount: %x\n", ntohs(hdr->arcount));
  
  len = sizeof(struct dns);

  for(int i =0; i < ntohs(hdr->qdcount); i++) {
    char *qn = (char *) (ibuf+len);
    qname = qn;
    decode_qname(qn, cc - len);
    len += strlen(qn)+1;
    len += sizeof(struct dns_question);
  }

  for(int i = 0; i < ntohs(hdr->ancount); i++) {
    if(len >= cc){
      printf("invalid DNS reply\n");
      exit(1);
    }
    
    char *qn = (char *) (ibuf+len);

    if((int) qn[0] > 63) {  // compression?
      qn = (char *)(ibuf+qn[1]);
      len += 2;
    } else {
      decode_qname(qn, cc - len);
      len += strlen(qn)+1;
    }
      
    struct dns_data *d = (struct dns_data *) (ibuf+len);
    len += sizeof(struct dns_data);
    //printf("type %d ttl %d len %d\n", ntohs(d->type), ntohl(d->ttl), ntohs(d->len));
    if(ntohs(d->type) == ARECORD && ntohs(d->len) == 4) {
      record = 1;
      printf("DNS arecord for %s is ", qname ? qname : "" );
      uint8 *ip = (ibuf+len);
      printf("%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
      if(ip[0] != 128 || ip[1] != 52 || ip[2] != 129 || ip[3] != 126) {
        printf("wrong ip address");
        exit(1);
      }
      len += 4;
    }
  }

  // needed for DNS servers with EDNS support
  for(int i = 0; i < ntohs(hdr->arcount); i++) {
    char *qn = (char *) (ibuf+len);
    if(*qn != 0) {
      printf("invalid name for EDNS\n");
      exit(1);
    }
    len += 1;

    struct dns_data *d = (struct dns_data *) (ibuf+len);
    len += sizeof(struct dns_data);
    if(ntohs(d->type) != 41) {
      printf("invalid type for EDNS\n");
      exit(1);
    }
    len += ntohs(d->len);
  }

  if(len != cc) {
    printf("Processed %d data bytes but received %d\n", len, cc);
    exit(1);
  }
  if(!record) {
    printf("Didn't receive an arecord\n");
    exit(1);
  }
}

static void
dns()
{
  //return;//suwei add
  #define N 1000
  uint8 obuf[N];
  uint8 ibuf[N];
  uint32 dst;
  int fd;
  int len;
  memset(obuf, 0, N);
  memset(ibuf, 0, N);
  
  // 8.8.8.8: google's name server
  dst = (8 << 24) | (8 << 16) | (8 << 8) | (8 << 0);

  if((fd = connect(dst, 10000, 53)) < 0){
    fprintf(2, "ping: connect() failed\n");
    exit(1);
  }

  len = dns_req(obuf);
  
  if(write(fd, obuf, len) < 0){
    fprintf(2, "dns: send() failed\n");
    exit(1);
  }
  int cc = read(fd, ibuf, sizeof(ibuf));
  if(cc < 0){
    fprintf(2, "dns: recv() failed\n");
    exit(1);
  }
  dns_rep(ibuf, cc);

  close(fd);
}  

int
main(int argc, char *argv[])
{
  int i, ret;
  uint16 dport = NET_TESTS_PORT;

  printf("nettests running on port %d\n", dport);
  
  printf("testing ping: ");
  #if improvedflag
  char *suweibuf=malloc(4096*2);//suwei: 2 pages, one for msg, one for mbuf  
  ping(2000, dport, 1, suweibuf);
  #else
  ping(2000, dport, 1);
  #endif
  printf("OK\n");  
  printf("testing single-process pings: ");  
  #if improvedflag
  char *suweibuf2=malloc(4096*1001);//suwei: 2 pages, one for msg, one for mbuf  
  uint64 timer1 = uptime();//suwei add
  for (i = 0; i < 1000; i++)
    ping(2000, dport, 1,suweibuf2+i*4096);
  #else
  uint64 timer1 = uptime();//suwei add
  for (i = 0; i < 1000; i++)
    ping(2000, dport, 1);
  #endif
  uint64 period = uptime()-timer1;//suwei add
  printf("OK\n");
  
  printf("single-process write time period: %d\n",period);//suwei add

  #if improvedflag//suwei add
  printf("testing multi-process pings: ");  
  timer1 = uptime();//suwei add
  for (i = 0; i < 10; i++){
    int pid = fork();
    if (pid == 0){
      char *suweibuf3=malloc(4096*2);//suwei: 2 pages, one for msg, one for mbuf
      ping(2000 + i + 1, dport, 1, suweibuf3);
      exit(0);
    }
  }
  #else
  printf("testing multi-process pings: ");
  timer1 = uptime();//suwei add
  for (i = 0; i < 10; i++){
    int pid = fork();
    if (pid == 0){
      ping(2000 + i + 1, dport, 1);
      exit(0);
    }
  }
  #endif

  for (i = 0; i < 10; i++){
    wait(&ret);
    if (ret != 0)
      exit(1);
  }
  period = uptime()-timer1;//suwei add
  printf("OK\n");  
  printf("multi-process write time period: %d\n",period);//suwei add


  printf("testing DNS\n");
  dns();
  printf("DNS OK\n");

  printf("all tests passed.\n");
  #if improvedflag
  free(suweibuf);//suwei add
  free(suweibuf2);//suwei add
  #endif
  exit(0);
}
