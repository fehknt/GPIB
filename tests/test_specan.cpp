#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "specan.h"
#include "gpiblib.h"
#include "comblock.h"
#include <string>
#include <vector>
#include <queue>
#include <iostream>

using namespace std;

// Mock ComBlock to intercept GPIB traffic from gpiblib
class MockSAComBlock : public COMBLOCK {
public:
    vector<string> sent_data;
    queue<string> next_responses;
    TERM_REASON next_term_reason = SR_EOS;
    bool ignore_flush = true;

    void add_response(string s) {
        next_responses.push(s);
    }

    void add_binary_response(const void* data, int len) {
        next_responses.push(string((const char*)data, len));
    }

    bool send(unsigned char* buffer, int len = -1, int timeout = INFINITE) override {
        string s((char*)buffer, len == -1 ? strlen((char*)buffer) : len);
        sent_data.push_back(s);
        cout << "DEBUG: SENT [" << s << "]" << endl;
        return true;
    }

    unsigned char* receive(int expected_len, int EOS_char, int timeout_msec, int dropout_msec,
                          SRPROGRESSCB progress_callback, int* actual_cnt, TERM_REASON* term_reason) override {
        static unsigned char buf[65536];
        
        // Correctly identify and ignore GPIB_flush_receive_buffers calls
        // Note: gpiblib.cpp uses -1 as expected_len and 250ms timeout for flushing
        if (ignore_flush && timeout_msec == 250) {
            if (actual_cnt) *actual_cnt = 0;
            if (term_reason) *term_reason = SR_TIMEOUT;
            return NULL;
        }

        if (next_responses.empty()) {
            if (actual_cnt) *actual_cnt = 0;
            if (term_reason) *term_reason = SR_TIMEOUT;
            cout << "DEBUG: RECV: EMPTY (TIMEOUT) [exp=" << expected_len << " eos=" << EOS_char << " tmo=" << timeout_msec << "]" << endl;
            return NULL;
        }
        string data = next_responses.front();
        next_responses.pop();
        
        int len = min((int)data.length(), 65535);
        memcpy(buf, data.c_str(), len);
        if (actual_cnt) *actual_cnt = len;
        if (term_reason) *term_reason = next_term_reason;
        
        cout << "DEBUG: RECV [" << data.substr(0, min((int)data.length(), 20)) << "] (" << len << " bytes)" << endl;
        
        return buf;
    }
};

class SpecanTest : public ::testing::Test {
protected:
    MockSAComBlock* mock;

    static void WINAPI dummy_handler(C8 *msg, S32 ibsta, S32 iberr, S32 ibcntl) {}

    void SetUp() override {
        mock = new MockSAComBlock();
        GPIB_set_serial_device(mock);
        SA_startup(); 
    }

    void TearDown() override {
        SA_disconnect();
        GPIB_set_serial_device(NULL);
    }
    
    void SetupHP8568Responses() {
        mock->add_response("Prologix version 6.0\n"); // For ++ver
        mock->add_response("0\n");                    // For ++auto query
        mock->add_response("HP8568B\n");              // For ID?
        mock->add_response("10\n");                   // LG?
        mock->add_response("0.0\n");                  // RL?
        mock->add_response("100.0\n");                // FA?
        mock->add_response("1000.0\n");               // FB?
        mock->add_response("1000.0\n");               // RB?
        mock->add_response("1000.0\n");               // VB?
        mock->add_response("off\n");                  // VAVG?
        mock->add_response("0.1\n");                  // ST?
        mock->add_response("10.0\n");                 // AT?
    }

    void SetupTek492Responses() {
        mock->add_response("Prologix version 6.0\n"); // For ++ver
        mock->add_response("0\n");                    // For ++auto query
        mock->add_response("ID TEK/492\n");           // For ID?
        mock->add_response("\n");                     // Consumption for SIGSWP; (Tek 490P SA_printf behavior)
        mock->add_response("VRTDSP LOG:10\n");        // VRTDSP?
        mock->add_response("\n");                     // Consumption for RLUNIT DBM;
        mock->add_response("REFLVL -10.0\n");         // REFLVL?
        mock->add_response("FREQ 1000000.0\n");       // FREQ?
        mock->add_response("SPAN 1000.0\n");          // SPAN?
        mock->add_response("RESBW 1000.0\n");         // RESBW?
        mock->add_response("VIDFLT WIDE\n");          // VIDFLT?
        mock->add_response("TIME 0.1\n");             // TIME?
    }
};

TEST_F(SpecanTest, DetectHP8568) {
    SetupHP8568Responses();
    ASSERT_TRUE(SA_connect(18, (C8*)"", dummy_handler));
    
    SA_STATE* state = SA_get_state(); 
    EXPECT_EQ(state->type, SATYPE_HP8566B_8568B);
}

TEST_F(SpecanTest, DetectTek492) {
    SetupTek492Responses();
    ASSERT_TRUE(SA_connect(1, (C8*)"", dummy_handler));
    
    SA_STATE* state = SA_get_state();
    EXPECT_EQ(state->type, SATYPE_TEK_490P);
}

TEST_F(SpecanTest, FetchTraceHP8568) {
    SetupHP8568Responses();
    ASSERT_TRUE(SA_connect(18, (C8*)"", dummy_handler));
    
    mock->add_response("1\n"); // spoll
    
    vector<U16> bin_data(1001);
    for(int i=0; i<1001; i++) {
        U16 val = (U16)i;
        bin_data[i] = ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
    }
    mock->add_binary_response(bin_data.data(), 1001 * 2);
    
    SA_fetch_trace();
    
    SA_STATE* state = SA_get_state();
    EXPECT_EQ(state->n_trace_points, 1001);
    ASSERT_TRUE(state->dBm_values != NULL);
    
    EXPECT_NEAR(state->dBm_values[0], -100.0, 0.1);
    EXPECT_NEAR(state->dBm_values[1000], 0.0, 0.1);
}
