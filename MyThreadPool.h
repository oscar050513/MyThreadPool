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
     explicit MyThreadPool(size_t n) : d_stop(false),d_threadnum(n) {}
     MyThreadPool(const MyThreadPool& other_tp) = delete;
     ~MyThreadPool();
     void init();
     template <typename F, typename... Args>
     auto submit_task(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
      {
        if(d_stop.load())
        {
          std::runtime_error("This threadpool has been stopped , can't accept new task");
        }
        using return_type = typename std::result_of<F(Args...)>::type;
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
  private:
    std::vector<std::unique_ptr<std::thread>> d_threads;
    size_t d_threadnum;
    std::queue<std::function<void()>> d_tasks;
    std::atomic<bool> d_stop;
    std::mutex d_mtx;
    std::condition_variable d_cv;
};

#endif
