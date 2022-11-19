#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int nthread = 1;
static int round = 0;

struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
}

static void 
barrier()
{
  // YOUR CODE HERE
  //
  // Block until all threads have called barrier() and
  // then increment bstate.round.
  //
  pthread_mutex_lock(&bstate.barrier_mutex);  // acquire lock

  bstate.nthread++;

  if(bstate.nthread == nthread){
    // 所有线程到达屏障
    bstate.round++;
    bstate.nthread = 0;  
    // 当最后一个线程broadcast之后，其他的线程从wait醒来竞争锁，但是实际上
    // 锁被当前这个broadcast的线程占着，其他线程均无法得到锁
    // 只有当这个线程完成所有变量的修改，主动释放锁，其他的线程才能抢得到锁
    // 这保证了出现某线程在其他线程退出barrier之前进入了下一轮循环不会出错
    // 因为某线程能退出barrier的前提是broadcast的那个线程已经完成所有变量的更新
    // 之后过快进入下一轮的线程nthread++是合法的
    pthread_cond_broadcast(&bstate.barrier_cond);
  }else{
    // 还有线程没到，当前线程：放锁+阻塞
    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
  }

  pthread_mutex_unlock(&bstate.barrier_mutex);  // release lock
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    int t = bstate.round;
    assert (i == t);
    barrier();
    usleep(random() % 100);
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
