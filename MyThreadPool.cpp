#include "MyThreadPool.h"

MyThreadPool::MyThreadPool(size_t n) noexcept : d_stop(false) {
  for(size_t idx=0;idx<n;++idx)
   {
     std::unique_ptr<std::thread> threadPtr(new std::thread(&MyThreadPool::do_work,this));
     auto tid = threadPtr->get_id();
     d_threads.emplace(tid,ThreadWrap(std::move(threadPtr),std::make_shared<std::atomic<bool>>(false)));
     //d_threads[tid] = {std::move(threadPtr),false};
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

bool resize(size_t targetCount) {
  return true;
}

void MyThreadPool::do_work()
{
  auto tid = std::this_thread::get_id();
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

