/**
  test.cpp (for ip_log.cpp)
  James Loving
**/

#include <unistd.h>   // for sleep()

#include "ip_log.cpp"

int main()
{
  // test ipv4_sublog
  ipv4_sublog ipv4_log(1000000, 0.001);
  std::cout << "Testing: ipv4_sublog\n";
  ipv4_log.add_connection("aa-bb-cc-dd-ee-ff", 10);
  std::cout << "  add_connection(\"aa-bb-cc-dd-ee-ff\", 10)\n";
  std::cout << "  has_connection: " << ipv4_log.has_connection("aa-bb-cc-dd-ee-ff", 10) << "\n";
  std::cout << "  sizeof(): " << sizeof(ipv4_log) << " bytes\n";
  // TODO: display current memory usage
  // TODO: delete ipv4_sublog?

  // test ip_log
  ip_log log;
  std::cout << "Testing: ip_log\n";
  sleep(1);
  time_t add_time_1 = time(nullptr);
  log.add_ipv4_connection("aa-bb-cc-dd-ee-ff", 10);
  std::cout << "  add_ipv4_connection(\"aa-bb-cc-dd-ee-ff\", 10): " << add_time_1 << "\n";
  std::cout << "  waiting 5 second(s) for sublog period to finish\n";
  sleep(5);
  time_t add_time_2 = time(nullptr);
  std::cout << "  add_ipv4_connection(\"bb-cc-dd-ee-ff-aa\", 10): " << add_time_2 << "\n";
  log.add_ipv4_connection("bb-cc-dd-ee-ff-aa", 10);
  std::cout << "  has_ipv4_connection(\"aa-bb-cc-dd-ee-ff\", 10): " << log.has_ipv4_connection("aa-bb-cc-dd-ee-ff", 10, add_time_1) << "\n";
  std::cout << "  has_ipv4_connection(\"bb-cc-dd-ee-ff-aa\", 10): " << log.has_ipv4_connection("bb-cc-dd-ee-ff-aa", 10, add_time_2) << "\n";
  std::cout << "  sizeof(): " << sizeof(log) << " bytes\n";

  return 0;
}
