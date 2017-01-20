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

// to allow Travis to test
#define _BSD_SOURCE
#define __FAVOR_BSD

#include <cstdlib>
#include <string>
#include <unistd.h>

#include "gtest/gtest.h"
#include "../edict.hpp"

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
    system("sudo mv /var/lib/edict/do_not_track.txt /var/lib/edict/do_not_track.txt.backup");
    system("sudo echo aabbccddeeff > /var/lib/edict/do_not_track.txt");

    device_log d;

    std::string s = "bbccddeeffgg";
    ASSERT_TRUE(d.should_log(s));

    s = "aabbccddeeff";
    ASSERT_FALSE(d.should_log(s));    

    system("mv /var/lib/edict/do_not_track.txt.backup /var/lib/edict/do_not_track.txt");
}

TEST(edict, parse_args)
{
    int arg_count = 2;
    char *arg_vector[10];

    // test "start" parsing
    arg_vector[0] = (char *)"edict";
    arg_vector[1] = (char *)"start";
    struct args_struct args = parse_args(arg_count, arg_vector);
    ASSERT_EQ("start", args.command);

    // test "help" parsing
    arg_count = 1;
    args = parse_args(arg_count, arg_vector);
    ASSERT_EQ("help", args.command);

    // test "query" parsing
    arg_vector[1] = (char *)"query";
    arg_vector[2] = (char *)"timestamp";
    arg_vector[3] = (char *)"v4";
    arg_vector[4] = (char *)"80";
    arg_vector[5] = (char *)"plain";
    arg_count = 6;
    args = parse_args(arg_count, arg_vector);
    ASSERT_EQ("query", args.command);
    ASSERT_EQ("timestamp", args.query_timestamp);

    // test that extra arguments triggers help
    arg_vector[6] = (char *)"samsung";
    arg_count = 7;
    args = parse_args(arg_count, arg_vector);
    ASSERT_EQ("invalid", args.command);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
