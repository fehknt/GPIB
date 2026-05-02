#include <gtest/gtest.h>
#include "gpib_utils.h"

TEST(GpibUtils, AscNum) {
    EXPECT_EQ(ascnum("123", 10), 123);
    EXPECT_EQ(ascnum("-456", 10), -456);
    EXPECT_EQ(ascnum("1A", 16), 26);
    
    C8* end = NULL;
    EXPECT_EQ(ascnum("123abc", 10, &end), 123);
    EXPECT_STREQ(end, "abc");
    
    EXPECT_EQ(ascnum("  789", 10), 789);
    EXPECT_EQ(ascnum(" -123", 10), -123);

}

TEST(GpibUtils, StriStr) {
    C8* text = (C8*)"The quick Brown Fox";
    EXPECT_NE(stristr(text, "BROWN"), (void*)NULL);
    EXPECT_EQ(stristr(text, "BROWN"), text + 10);
    
    EXPECT_NE(stristr("test-string", "TEST_STRING", true), (void*)NULL);
}

TEST(GpibUtils, KillWhitespace) {
    C8 buf[64];
    strcpy(buf, "  test  \n\r ");
    kill_trailing_whitespace(buf);
    EXPECT_STREQ(buf, "  test");
}
