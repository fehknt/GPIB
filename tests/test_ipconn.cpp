#include "ipconn.h"
#include <gtest/gtest.h>

TEST(IPConn, Init) {
    IPCONN conn;
    // Basic check that we can instantiate and maybe call a simple non-networked method
    // IPCONN doesn't have many non-networked methods.
    
    // We can try to resolve a local address
    // ASSERT_TRUE(conn.resolve("127.0.0.1", 80));
}
