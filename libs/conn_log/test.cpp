/*
  test.cpp (for ip_log.cpp)
  James H. Loving
*/

#include <iostream>
#include <string>
#include <time.h>
#include <unistd.h>   // for sleep()

#include "conn_log.cpp"

int main()
{
    conn_log log;

    std::string mac = "aa:bb:cc:dd:ee:ff";

    std::cout << "\nTest add_ipv4:\n";

    log.add_ipv4(mac, 17500);

    std::cout << "\nTest has_ipv4:\n";

    std::cout << log.has_ipv4(mac,
                              17,
                              time(nullptr));
    
    std::cout << "\n\nTests complete.\n";

    return 0;
}
