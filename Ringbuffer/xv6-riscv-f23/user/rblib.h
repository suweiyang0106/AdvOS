//suwei created the head

struct user_ring_buf {
   //find memory when compiler
   void *buf;
   void *book;
   int exists;
};
void test(void);//suwei add
void ringbuf_start_read(int ring_desc, char **addr, int *bytes);
void ringbuf_finish_read(int ring_desc, int bytes);
void ringbuf_start_write(int ring_desc, char **addr, int *bytes);
void ringbuf_finish_write(int ring_desc, int bytes);
