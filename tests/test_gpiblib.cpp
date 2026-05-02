#include <gtest/gtest.h>
#include "gpiblib.h"
#include "comblock.h"
#include <vector>
#include <string>

class MockComBlock : public COMBLOCK {
public:
    std::vector<std::string> sent_data;
    std::string next_receive_data;
    TERM_REASON next_term_reason = SR_EOS;
    bool should_fail_send = false;
    bool should_fail_receive = false;

    bool send(unsigned char *buffer, int len, int timeout) override {
        if (should_fail_send) return false;
        if (len == -1) len = (int)strlen((char*)buffer);
        sent_data.push_back(std::string((char*)buffer, len));
        return true;
    }

    unsigned char *receive(int expected_len, int EOS_char, int timeout_msec, int dropout_msec, 
                          SRPROGRESSCB progress_callback, int *actual_cnt, TERM_REASON *term_reason) override {
        if (should_fail_receive) {
            if (actual_cnt) *actual_cnt = 0;
            if (term_reason) *term_reason = SR_TIMEOUT;
            return NULL;
        }
        static unsigned char buf[65536];
        int len = (int)std::min((size_t)65535, next_receive_data.size());
        if (len > 0) {
            memcpy(buf, next_receive_data.c_str(), len);
            next_receive_data.clear(); // Consume the data
            if (actual_cnt) *actual_cnt = len;
            if (term_reason) *term_reason = next_term_reason;
        } else {
            if (actual_cnt) *actual_cnt = 0;
            if (term_reason) *term_reason = SR_TIMEOUT;
        }
        return buf;
    }
};

class GpibLibTest : public ::testing::Test {
protected:
    MockComBlock* mock;
    std::string current_com;
    static int test_count;

    static void WINAPI dummy_handler(C8 *msg, S32 ibsta, S32 iberr, S32 ibcntl) {
        // Do nothing
    }

    void SetUp() override {
        test_count++;
        current_com = "COM" + std::to_string(test_count + 10);
        mock = new MockComBlock();
        GPIB_set_serial_device(mock);
    }

    void TearDown() override {
        GPIB_disconnect(); 
        GPIB_set_serial_device(NULL);
    }
};

int GpibLibTest::test_count = 0;

TEST_F(GpibLibTest, UsbGpibWithIDQuery) {
    mock->next_receive_data = "Xyphro UsbGpib version 1.0\r\n";
    ASSERT_TRUE(GPIB_connect_ex((C8*)(current_com + ":15").c_str(), dummy_handler, false, 100));
    
    bool found_ver = false;
    for (const auto& s : mock->sent_data) {
        if (s.find("++ver") != std::string::npos) found_ver = true;
    }
    EXPECT_TRUE(found_ver);
}

TEST_F(GpibLibTest, UsbGpibNoIDQuery) {
    mock->next_receive_data = "";
    mock->next_term_reason = SR_TIMEOUT;
    
    ASSERT_TRUE(GPIB_connect_ex((C8*)(current_com + ":15").c_str(), dummy_handler, false, 100));
    
    GPIB_write((C8*)"*IDN?");
    
    bool found_idn = false;
    for (const auto& s : mock->sent_data) {
        if (s.find("*IDN?") != std::string::npos) found_idn = true;
    }
    EXPECT_TRUE(found_idn);
}

TEST_F(GpibLibTest, Formatting) {
    mock->next_receive_data = "dummy";
    ASSERT_TRUE(GPIB_connect_ex((C8*)(current_com + ":15").c_str(), dummy_handler, false, 100));
    
    GPIB_printf((C8*)"SET %d, %.2f  ", 42, 3.14159);
    
    ASSERT_FALSE(mock->sent_data.empty());
    EXPECT_EQ("SET 42, 3.14\r\n", mock->sent_data.back());
}

TEST_F(GpibLibTest, ErrorHandling) {
    ASSERT_TRUE(GPIB_connect_ex((C8*)(current_com + ":15").c_str(), dummy_handler, false, 100));
    
    mock->should_fail_send = true;
    EXPECT_EQ(GPIB_write((C8*)"FAIL"), 0);
}

TEST_F(GpibLibTest, TimeoutHandling) {
    ASSERT_TRUE(GPIB_connect_ex((C8*)(current_com + ":15").c_str(), dummy_handler, false, 100));
    
    mock->should_fail_receive = true;
    C8* res = GPIB_read_ASC(100, true);
    EXPECT_TRUE(res == NULL); 
}

TEST_F(GpibLibTest, ConnectionType) {
    mock->next_receive_data = "Prologix version 6.0\r\n";
    ASSERT_TRUE(GPIB_connect_ex((C8*)(current_com + ":15").c_str(), dummy_handler, false, 100));
    
    EXPECT_EQ(GPIB_connection_type(), GC_PROLOGIX_SERIAL);
}

TEST_F(GpibLibTest, ReadASC) {
    ASSERT_TRUE(GPIB_connect_ex((C8*)(current_com + ":15").c_str(), dummy_handler, false, 100));
    mock->next_receive_data = "DATA\r\n";
    C8* res = GPIB_read_ASC(-1, FALSE, FALSE);
    ASSERT_TRUE(res != NULL);
    EXPECT_STREQ(res, "DATA\r\n");
}

TEST_F(GpibLibTest, SerialPoll) {
    mock->next_receive_data = "Prologix version 6.0\r\n";
    ASSERT_TRUE(GPIB_connect_ex((C8*)(current_com + ":15").c_str(), dummy_handler, false, 100));
    
    mock->next_receive_data = "64\r\n"; 
    mock->sent_data.clear();
    
    U8 spoll = GPIB_serial_poll();
    EXPECT_EQ(spoll, 64);
}
