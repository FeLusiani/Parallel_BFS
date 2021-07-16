#include <chrono>
#include <string>

class utimer {
  std::chrono::system_clock::time_point start;
  std::chrono::system_clock::time_point stop;
  std::string message; 
  using usecs = std::chrono::microseconds;
  using msecs = std::chrono::milliseconds;

private:
  long * us_elapsed;
  
public:
  utimer(const std::string m, long * us = (long *)NULL);
  ~utimer();
};

class my_timer {
  std::chrono::system_clock::time_point start;

  using usecs = std::chrono::microseconds;
  using msecs = std::chrono::milliseconds;

private:
  long * us_elapsed;
  
public:
  my_timer();
  long int get_time();
};