/*
  test.cpp (for conn_log.cpp)
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

    std::cout << "\nTest has_ipv4 (should be 10):\n";

    std::cout << log.has_ipv4(mac, 17500, time(nullptr));
    std::cout << log.has_ipv4(mac, 17, time(nullptr));

    std::cout << "\n\nTest add_ipv6:\n";

    log.add_ipv6(mac, "::1");

    std::cout << "\nTest has_ipv6 (should be 10):\n";

    std::cout << log.has_ipv6(mac, "::1", time(nullptr));
    std::cout << log.has_ipv6(mac, "::2", time(nullptr));
    
    std::cout << "\n\nTests complete.\n";

    return 0;
}
