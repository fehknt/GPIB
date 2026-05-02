#include "7470lex.h"
#include <gtest/gtest.h>

TEST(Lex7470, Basic) {
    char data[] = "IN;PU;PA100,200;PD;PA300,400;";
    plot_data = data;
    plot_end = data + strlen(data);
    ptr = data;
    
    ASSERT_EQ(fetch_command(), CMD_IN);
    ASSERT_EQ(fetch_command(), CMD_PU);
    ASSERT_EQ(fetch_command(), CMD_PA);
    ASSERT_EQ(next_number(), 1); // True
    ASSERT_EQ(fetch_integer(), 100);
    fetch_comma();
    ASSERT_EQ(fetch_integer(), 200);
    ASSERT_EQ(fetch_command(), CMD_PD);
    ASSERT_EQ(fetch_command(), CMD_PA);
    ASSERT_EQ(fetch_integer(), 300);
    fetch_comma();
    ASSERT_EQ(fetch_integer(), 400);
    ASSERT_EQ(fetch_command(), CMD_END_OF_DATA);
}

TEST(Lex7470, Numbers) {
    char data[] = "123,-456,7.89;";
    plot_data = data;
    plot_end = data + strlen(data);
    ptr = data;
    
    ASSERT_EQ(fetch_integer(), 123);
    fetch_comma();
    ASSERT_EQ(fetch_integer(), -456);
    fetch_comma();
    ASSERT_NEAR(fetch_float(), 7.89, 0.001);
    
    char data2[] = "PA  100,  -200;";
    plot_data = data2;
    plot_end = data2 + strlen(data2);
    ptr = data2;
    
    ASSERT_EQ(fetch_command(), CMD_PA);
    ASSERT_EQ(fetch_integer(), 100);
    fetch_comma();
    ASSERT_EQ(fetch_integer(), -200);
}

