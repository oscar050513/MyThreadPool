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
     std::cout<<"thread "<<tid<<" joined upon threadpool dtor"<<std::endl;
     threadwrap.threadPtr->join();
  }  
}

void MyThreadPool::add_worker()
{
  std::unique_ptr<std::thread> threadPtr(new std::thread(&MyThreadPool::do_work,this));
  auto tid = threadPtr->get_id();
  //WARNING , cout output tid will cause crash
  //std::cout<<"launched worker thread : "<<tid<<std::endl;
  //std::this_thread::sleep_for(std::chrono::seconds(5));
  d_threads.emplace(tid,ThreadWrap(std::move(threadPtr),std::shared_ptr<std::atomic<int>>(new std::atomic<int>(0))));
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
    std::cout<<"remove threads in status : "<<targetStatus<<std::endl;
    for(auto& [tid,threadWrap] : this->d_threads)
    {
        if(toRemoveCount<=0)
        {
          break;
        }
        std::cout<<"check status of worker thread: "<<tid<<std::endl;
        std::cout<<threadWrap.status->load()<<std::endl;
        std::cout<<"done check status of worker thread: "<<tid<<std::endl;
        if(threadWrap.status->load()== targetStatus)
        {
          std::cout<<"remove thread "<<tid<<" triggered by threadpool resize"<<std::endl;
          --toRemoveCount;
          threadWrap.status->store(2);
          threadWrap.threadPtr->detach();
          std::cout<<"remove thread from data structure"<<std::endl;
          this->d_threads.erase(tid);
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

void MyThreadPool::do_work()
{
  auto tid = std::this_thread::get_id();
  std::cout<<"worker thread "<<tid<<" launched to process patch"<<std::endl;
  auto statusPtr = d_threads[tid].status;
  while(true)
  {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lck(d_mtx);
      d_cv.wait(lck,[this,statusPtr](){return statusPtr->load()==2||this->d_stop.load() || !this->d_tasks.empty();});
      //d_cv.wait(lck,[this,statusPtr](){return this->d_stop.load() || !this->d_tasks.empty();});
      if(d_stop.load()&&d_tasks.empty() || statusPtr->load()==2)
      {
        std::cout<<"thread "<<tid<<" return and lifecycle end"<<std::endl;
        return;
      }
     task = std::move(d_tasks.front());
     d_tasks.pop();
    }
    std::cout<<"thread "<<tid<<" start to process a task"<<std::endl;
    statusPtr->store(1);
    task();
    statusPtr->store(0);
    std::cout<<"thread "<<tid<<" complete processing a task"<<std::endl;
  }
}

