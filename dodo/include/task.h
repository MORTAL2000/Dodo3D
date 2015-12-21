
#pragma once

#include <vector>
#include <pthread.h>

namespace Dodo
{

struct ITask
{
  ITask();
  virtual ~ITask();
  bool DependenciesRemaining();
  void DependsOn( ITask* task );
  void ClearOneDependency();
  virtual void Run() = 0;

  std::vector<ITask*> mDependentTask;
  volatile int        mDependenciesRemaining;
  bool                mComplete;
};

class ThreadPool
{
public:

  ThreadPool( unsigned int numThreads );
  ~ThreadPool();
  void AddTask( ITask* task );
  ITask* GetNextTask();

  void Exit();

  void WaitForCompletion();
private:

  struct WorkerThread
  {
    WorkerThread(ThreadPool* pool, int core);
    ~WorkerThread();
    static void* Run( void* data );

    ThreadPool* mPool;    //Pointer to thread pool
    pthread_t   mThread;  //Thread
    bool        mExit;
  };

  std::vector<WorkerThread*>  mWorkerThread;  //Worker threads
  std::vector<ITask*>         mTask;          //list of tasks
  pthread_mutex_t             mLock;
  pthread_cond_t              mCondition;
  pthread_cond_t              mCondition2;
  size_t                      mWaitingThreads;
  bool                        mExit;
};

}

