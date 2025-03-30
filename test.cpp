#include <iostream>
#include "MyThreadPool.h"

void test_task()
{
  //std::cout<<"this task is executed by thread"<<std::this_thread::get_id()<<std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(2));
  //std::cout<<"task execution complete"<<std::endl;
}

int main()
{
  MyThreadPool tp(8);
  for(int i=0;i<80;++i)
  {
   tp.submit_task(test_task);
  }
  std::this_thread::sleep_for(std::chrono::seconds(4));
  tp.resize(2);
  std::this_thread::sleep_for(std::chrono::seconds(20));
  tp.resize(4);
  std::this_thread::sleep_for(std::chrono::seconds(20));
  return 0;
}
