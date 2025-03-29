#include "MyThreadPool.h"

MyThreadPool::~MyThreadPool()
{
   d_stop.store(true);
   d_cv.notify_all();
   for(auto& threadPtr : d_threads)
  {
     threadPtr->join();
  }  
}

void MyThreadPool::init()
{
   for(size_t idx=0;idx<d_threadnum;++idx)
   {
     d_threads.emplace_back(std::make_unique<std::thread>(std::thread(&MyThreadPool::do_work,this)));
   }
}

void MyThreadPool::do_work()
{
  while(true)
  {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lck(d_mtx);
      d_cv.wait(lck,[this](){return this->d_stop.load() || !this->d_tasks.empty();});
      if(d_stop.load()&&d_tasks.empty())
      {
        return;
      }
     task = std::move(d_tasks.front());
     d_tasks.pop();
    }
    task();
  }
}

