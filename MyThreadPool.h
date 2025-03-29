#ifndef MyThreadPool_H
#define MyThreadPool_H

#include <vector>
#include <mutex>
#include <thread>
#include <future>
#include <condition_variable>
#include <queue>
#include <functional>
#include <type_traits>
#include <atomic>
#include <stdexcept>

class MyThreadPool {
  public:
     explicit MyThreadPool(size_t n) noexcept;
     MyThreadPool(const MyThreadPool& other_tp) = delete;
     ~MyThreadPool() noexcept;
     //void init();
     bool resize(size_t targetCount);
     template <typename F, typename... Args>
     auto submit_task(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
      {
        if(d_stop.load())
        {
          std::runtime_error("This threadpool has been stopped , can't accept new task");
        }
        using return_type = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f),std::forward<Args>(args)...)); 
        std::future<return_type> ret = task->get_future();
        {
          std::lock_guard<std::mutex> lck(d_mtx);
          d_tasks.push([task](){(*task)();});
          d_cv.notify_all();
        }   
        return ret;
      }
  private:
    void do_work(); 
    void add_worker();
  private:
    struct ThreadWrap {
       public:
         ThreadWrap() = default;
         ~ThreadWrap() = default;
         ThreadWrap(std::unique_ptr<std::thread> tPtr,std::shared_ptr<std::atomic<int>> s)
         {
           threadPtr = std::move(tPtr);
           status = s;
         }
         ThreadWrap(ThreadWrap&& other) noexcept
         {
           threadPtr = std::move(other.threadPtr);
           status = other.status;
         }
         std::unique_ptr<std::thread> threadPtr;
         //0:idle , 1:busy, 2: toRemove
         std::shared_ptr<std::atomic<int>> status;
    };
    std::unordered_map<std::thread::id,ThreadWrap> d_threads;
    std::queue<std::function<void()>> d_tasks;
    std::atomic<bool> d_stop;
    std::mutex d_mtx;
    std::condition_variable d_cv;
};

#endif
