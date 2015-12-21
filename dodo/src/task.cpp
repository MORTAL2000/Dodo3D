#include <task.h>


using namespace Dodo;

/**
 * WorkerThread
 */
ThreadPool::WorkerThread::WorkerThread(ThreadPool* pool, int core)
:mPool(pool),
 mExit(false)
{
  pthread_create( &mThread, NULL, Run, this);

  //Set thread affinity
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  pthread_setaffinity_np(mThread, sizeof(cpu_set_t), &cpuset);  
}

ThreadPool::WorkerThread::~WorkerThread()
{}

void* ThreadPool::WorkerThread::Run( void* context )
{
  WorkerThread* workerThread = (WorkerThread*)context;
  while( !workerThread->mExit )
  {
    ITask* task = workerThread->mPool->GetNextTask();
    if( task )
    {
      if( task->DependenciesRemaining() )
      {
        //Add the task to the pool as it can not complete while dependencies are missing
        workerThread->mPool->AddTask( task );
        continue;
      }

      //Execute task
      task->Run();

      //Signal dependent tasks
      size_t count( task->mDependentTask.size() );
      for( size_t i(0); i<count; ++i )
      {
        task->mDependentTask[i]->ClearOneDependency();
      }

      task->mComplete = true;
    }
  }

  return 0;
}

/**
 * ThreadPool
 */
ThreadPool::ThreadPool( unsigned int numThreads )
:mWaitingThreads(0),
 mExit(false)
{
  //Create a mutex
  pthread_mutex_init(&mLock, NULL);
  pthread_cond_init(&mCondition, 0);

  //Create worker threads
  mWorkerThread.resize( numThreads );
  for( unsigned int i(0); i<numThreads; ++i )
  {
    mWorkerThread[i] = new WorkerThread(this, i+1);
  }
}

ThreadPool::~ThreadPool()
{
}

ITask* ThreadPool::GetNextTask()
{
  ITask* task(0);

  pthread_mutex_lock(&mLock);

  while( mTask.empty() && !mExit )
  {
    mWaitingThreads++;
    pthread_cond_wait(&mCondition, &mLock);
    mWaitingThreads--;
  }

  if( mExit )
  {
    task = 0;
  }
  else
  {
    task = mTask[0];
    mTask.erase(mTask.begin());
  }

  pthread_mutex_unlock(&mLock);

  return task;
}

void ThreadPool::AddTask( ITask* task )
{
  task->mComplete = false;
  pthread_mutex_lock(&mLock);
  mTask.push_back(task);
  pthread_mutex_unlock(&mLock);
  pthread_cond_signal(&mCondition);
}

void ThreadPool::Exit()
{
  for( unsigned int i(0); i<mWorkerThread.size(); ++i )
  {
    mWorkerThread[i]->mExit = true;
  }

  pthread_mutex_lock(&mLock);
  mExit = true;
  pthread_mutex_unlock(&mLock);
  pthread_cond_broadcast(&mCondition);

  for( unsigned int i(0); i<mWorkerThread.size(); ++i )
  {
    pthread_join( mWorkerThread[i]->mThread, 0 );
  }
}

void ThreadPool::WaitForCompletion()
{
  while( mWaitingThreads < mWorkerThread.size() )
  {}
}
/**
 * ITask
 */
ITask::ITask()
:mDependenciesRemaining(0),
 mComplete(false)
{}

ITask::~ITask()
{}

bool ITask::DependenciesRemaining()
{
  return mDependenciesRemaining != 0;
}

void ITask::DependsOn( ITask* task )
{
  task->mDependentTask.push_back(this);
  mDependenciesRemaining++;
}

void ITask::ClearOneDependency()
{
  //@TODO: mDependenciesRemaining can be accesed from multiple threads
  --mDependenciesRemaining;
}




