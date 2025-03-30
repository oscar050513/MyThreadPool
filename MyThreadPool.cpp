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
   for(auto& [tid,threadwrap] : d_threads)
  {
     threadwrap.threadPtr->join();
  }  
}

void MyThreadPool::add_worker()
{
  auto tid = d_tid_cur;
  d_threads.emplace(d_tid_cur++,ThreadWrap(std::unique_ptr<std::thread>(new std::thread(&MyThreadPool::do_work,this,tid)),std::shared_ptr<std::atomic<int>>(new std::atomic<int>(0))));
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
    for(auto iter=this->d_threads.begin();iter!=this->d_threads.end();)
    {
        if(toRemoveCount<=0)
        {
          break;
        }
        auto tid = iter->first;
        if(iter->second.status->load()== targetStatus)
        {
          std::cout<<"remove thread "<<tid<<" triggered by threadpool resize"<<std::endl;
          --toRemoveCount;
          iter->second.status->store(2);
          iter->second.threadPtr->detach();
          std::cout<<"remove thread from data structure"<<std::endl;
          iter = this->d_threads.erase(iter);
          std::cout<<"done removed thread from data structure: "<<toRemoveCount<<std::endl;
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

void MyThreadPool::do_work(std::size_t tid)
{
  std::cout<<"worker thread "<<tid<<" launched to process patch"<<std::endl;
  auto statusPtr = d_threads[tid].status;
  while(true)
  {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lck(d_mtx);
      d_cv.wait(lck,[this,statusPtr](){return statusPtr->load()==2||this->d_stop.load() || !this->d_tasks.empty();});
      if(d_stop.load()&&d_tasks.empty() || statusPtr->load()==2)
      {
        std::cout<<"thread "<<tid<<" return and lifecycle end"<<std::endl;
        return;
      }
     task = std::move(d_tasks.front());
     d_tasks.pop();
    }
    std::cout<<"thread "<<tid<<" start to process a task"<<std::endl;
    if(statusPtr->load()!=2)
   {
    statusPtr->store(1);
   }
    task();
   if(statusPtr->load()!=2)
   {
    statusPtr->store(0);
   }
    std::cout<<"thread "<<tid<<" complete processing a task"<<std::endl;
  }
}

