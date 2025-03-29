#include "MyThreadPool.h"
#include <iostream>

MyThreadPool::MyThreadPool(size_t n) noexcept : d_stop(false) {
  for(size_t idx=0;idx<n;++idx)
   {
     add_worker();
   }
};

MyThreadPool::~MyThreadPool() noexcept
{
   d_stop.store(true);
   d_cv.notify_all();
   for(auto& [_,threadwrap] : d_threads)
  {
     threadwrap.threadPtr->join();
  }  
}

void MyThreadPool::add_worker()
{
  std::unique_ptr<std::thread> threadPtr(new std::thread(&MyThreadPool::do_work,this));
  auto tid = threadPtr->get_id();
  d_threads.emplace(tid,ThreadWrap(std::move(threadPtr),std::make_shared<std::atomic<int>>(false)));
}

bool MyThreadPool::resize(size_t targetCount) {
  int n = d_threads.size();
  bool ret = true;
  if(targetCount > n)
  {
   size_t toAdd = targetCount - n;
   while(toAdd-- > 0)
   {
     add_worker();
   }
  }
  else if(targetCount < n)
  {
    size_t toRemoveCount = n - targetCount;
    auto removeThread = [&toRemoveCount,this](int targetStatus) {
    for(auto& [tid,threadWrap] : this->d_threads)
    {
        if(toRemoveCount<=0)
        {
          break;
        }
        if(threadWrap.status->load()== targetStatus)
        {
          --toRemoveCount;
          threadWrap.status->store(2);
          threadWrap.threadPtr->detach();
          this->d_threads.erase(tid);
        }
     }
    };
    //first scan to remove idle thread first
    removeThread(0);
    //second scan to remove busy thread if needed
    removeThread(1);
    if(toRemoveCount>0)
    {
      std::cout<<"fail to remove target count of worker threads"<<std::endl;
      ret = false;
    }   
 
    d_cv.notify_all();  
  }
  return ret;
}

void MyThreadPool::do_work()
{
  auto tid = std::this_thread::get_id();
  auto statusPtr = d_threads[tid].status;
  while(true)
  {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lck(d_mtx);
      d_cv.wait(lck,[this,statusPtr](){return statusPtr->load()==2||this->d_stop.load() || !this->d_tasks.empty();});
      if(d_stop.load()&&d_tasks.empty() || statusPtr->load()==2)
      {
        return;
      }
     task = std::move(d_tasks.front());
     statusPtr->store(1);
     d_tasks.pop();
     statusPtr->store(0);
    }
    task();
  }
}

