#include <iostream>
#include <chrono>
#include "utimer.hpp"

utimer::utimer(const std::string m, long *us) : message(m), us_elapsed(us)
{
  start = std::chrono::system_clock::now();
}

utimer::~utimer()
{
  stop = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed = stop - start;
  auto musec = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

  std::cout << message << " computed in " << musec << " usec " << std::endl;
  if (us_elapsed != NULL)
    (*us_elapsed) = musec;
}

my_timer::my_timer(){
  start = std::chrono::system_clock::now();
}

long int my_timer::get_time(){
  auto stop = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed = stop - start;
  return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}