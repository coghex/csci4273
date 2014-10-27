#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <stdlib.h>

class ThreadPool
{
  public:
    ThreadPool(size_t threadCount);
    ~ThreadPool( );
    int dispatch_thread(void dispatch_function(void *), void *arg);
    bool thread_avail( );
  private:
    struct funk {
      void * f;
      void * a;
    };
    pthread_t *threads;
    void *threadstart(void * arg);
    static void *ihateclasses(void * context)
    {
      return ((ThreadPool *)context)->threadstart(context);
    }
    std::queue<funk> q;
    sem_t sem;
    int max;
};

void *ThreadPool::threadstart(void * arg)
{
  while (1) {
    sem_wait(&sem);
    struct funk adelic = q.front();
    // silly c
    void (* f)(void *) = (void (*)(void *))adelic.f;
    q.pop();
    f(adelic.a);
  }
}


ThreadPool::ThreadPool(size_t threadCount)
{
  sem_init(&sem, 0, -1);
  int i;
  threads = (pthread_t *)malloc(threadCount*sizeof(pthread_t));
  for(i=0;i<threadCount;i++) {
    pthread_create(&threads[i], NULL, &ThreadPool::ihateclasses, NULL);
  }
  max = threadCount;
}

ThreadPool::~ThreadPool( )
{
  int i;
  for(i=0;i<max;i++) {
    pthread_join(threads[i], NULL);
  }
  free(threads);
  sem_destroy(&sem);
}

int ThreadPool::dispatch_thread(void dispatch_function(void *), void *arg)
{
  struct funk f;
  f.a = arg;
  f.f = (void *)dispatch_function;
  q.push(f);
  sem_post(&sem);
  return 0;
}

bool ThreadPool::thread_avail( )
{
  int threads;
  sem_getvalue(&sem, &threads);
  if (threads >= max) {
    return false;
  }
  else {
    return true;
  }
}
