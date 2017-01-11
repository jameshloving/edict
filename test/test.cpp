//=============================================================================
//
// Name:        test.cpp
// Authors:     James H. Loving
// Description: This file defines all unit tests for the non-interface
//              portions of EDICT.
//
//              To run manually:
//              - cmake CMakeLists.txt
//              - make
//              - ./runTests
//
//=============================================================================

#include <cstdlib>
#include <string>
#include <unistd.h>

#include "gtest/gtest.h"
#include "../libs/conn_log/conn_log.hpp"
#include "../libs/device_log/device_log.hpp"

TEST(conn_log, valid_mac)
{
    system("/home/ubuntu/edict/libs/bloomd/bloomd -f /home/ubuntu/edict/libs/bloomd/bloomd.conf > /dev/null &");
    sleep(1);

    conn_log c;

    std::string s = "aabbccddeeff";
    ASSERT_TRUE(c.valid_mac(s));

    s = "001122334455";
    ASSERT_TRUE(c.valid_mac(s));

    s = "aa11bb22cc33";
    ASSERT_TRUE(c.valid_mac(s));

    s = "aa:bb:cc:dd:ee:ff";
    ASSERT_FALSE(c.valid_mac(s));

    s = "/abbccddeeff";
    ASSERT_FALSE(c.valid_mac(s));
    
    s = ":abbccddeeff";
    ASSERT_FALSE(c.valid_mac(s));

    s = "AABBCCDDEEFF";
    ASSERT_FALSE(c.valid_mac(s));

    s = "ggbbccddeeff";
    ASSERT_FALSE(c.valid_mac(s));

    s = "aabbccddeeff\n";
    ASSERT_FALSE(c.valid_mac(s));

    system("pkill bloomd > /dev/null");
}

TEST(conn_log, valid_ipv6)
{
     system("/home/ubuntu/edict/libs/bloomd/bloomd -f /home/ubuntu/edict/libs/bloomd/bloomd.conf > /dev/null &");
    sleep(1);

    conn_log c;

    std::string s = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";
    ASSERT_TRUE(c.valid_ipv6(s));

    s = "2001:db8:85a3::8a2e:370:7334";
    ASSERT_TRUE(c.valid_ipv6(s));

    s = "::1::2";
    ASSERT_FALSE(c.valid_ipv6(s));

    s = "127.0.0.1";
    ASSERT_FALSE(c.valid_ipv6(s));

    system("pkill bloomd > /dev/null");
}

TEST(device_log, should_log)
{
    system("mv /home/ubuntu/edict/stor/dnt.txt /home/ubuntu/edict/stor/dnt.txt.backup");
    system("echo aabbccddeeff > /home/ubuntu/edict/stor/dnt.txt");

    device_log d;

    std::string s = "bbccddeeffgg";
    ASSERT_TRUE(d.should_log(s));

    s = "aabbccddeeff";
    ASSERT_FALSE(d.should_log(s));    

    system("mv /home/ubuntu/edict/stor/dnt.txt.backup /home/ubuntu/edict/stor/dnt.txt");
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
