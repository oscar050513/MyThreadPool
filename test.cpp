#include <iostream>
#include "MyThreadPool.h"

void test_task()
{
  std::cout<<"this task is executed by thread"<<std::this_thread::get_id()<<std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::cout<<"task execution complete"<<std::endl;
}

int main()
{
  MyThreadPool tp(5);
  for(int i=0;i<20;++i)
  {
   tp.submit_task(test_task);
  }
  return 0;
}
