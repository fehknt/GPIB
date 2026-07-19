#include "ipconn.h"
#include <gtest/gtest.h>

//
// Most of IPCONN is inherently networked -- connect(), send_block(),
// read_block() and the IPSERVER/UDPXCVR machinery all need a live peer, so
// they can't be covered by a self-contained unit test.  The two static address
// helpers, though, are pure formatting/parsing of numeric IPv4 addresses and
// run entirely on the local host.  Round-trip them.
//
// parse_address() resolves numeric dotted-quads locally (inet_pton), so no DNS
// traffic is generated; we deliberately avoid hostname inputs, which would.
//

class IPConnAddr : public ::testing::Test {
protected:
    // The address helpers live in ws2_32, which needs Winsock initialized
    void SetUp()    override { WSADATA w; ASSERT_EQ(WSAStartup(MAKEWORD(2, 2), &w), 0); }
    void TearDown() override { WSACleanup(); }
};

TEST_F(IPConnAddr, ParsesPortSuffix) {
    sockaddr_in a;
    ASSERT_TRUE(IPCONN::parse_address((C8 *)"127.0.0.1:1234", 80, &a));
    EXPECT_EQ(a.sin_family, AF_INET);
    EXPECT_EQ(ntohs(a.sin_port), 1234);        // explicit port wins over the default
}

TEST_F(IPConnAddr, UsesDefaultPortWhenAbsent) {
    sockaddr_in a;
    ASSERT_TRUE(IPCONN::parse_address((C8 *)"192.168.1.50", 8080, &a));
    EXPECT_EQ(ntohs(a.sin_port), 8080);        // no ':port' -> caller's default
}

TEST_F(IPConnAddr, RoundTripsThroughAddressString) {
    sockaddr_in a;
    ASSERT_TRUE(IPCONN::parse_address((C8 *)"10.0.0.5:9000", 80, &a));

    C8 text[128];
    IPCONN::address_string(&a, text, sizeof(text), TRUE);
    EXPECT_STREQ(text, "10.0.0.5:9000");

    IPCONN::address_string(&a, text, sizeof(text), FALSE);
    EXPECT_STREQ(text, "10.0.0.5");            // include_port = FALSE drops the suffix
}

TEST_F(IPConnAddr, OmitsZeroPort) {
    sockaddr_in a;
    ASSERT_TRUE(IPCONN::parse_address((C8 *)"172.16.0.1", 0, &a));

    C8 text[128];
    IPCONN::address_string(&a, text, sizeof(text), TRUE);
    EXPECT_STREQ(text, "172.16.0.1");          // a zero port is never appended
}
