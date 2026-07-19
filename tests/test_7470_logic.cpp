#include <gtest/gtest.h>
#include <gmock/gmock.h>

// BUILDING_GPIBLIB / BUILDING_SPECAN come from gpib_common's PUBLIC compile
// definitions; defining them here too just produces C4005

#include "7470_logic.h"
#include "gpiblib.h"
#include "specan.h"
#include <string.h>

using ::testing::Return;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::InSequence;

void __cdecl GPIB_print(C8 *fmt, ...) {}

// Stubs for functions used by synthesizers or logic but not mocked
extern "C" {
    C8* WINAPI GPIB_read_ASC(S32 max_len, bool report_timeout, bool from_board) { return (C8*)""; }
    C8* WINAPI GPIB_read_BIN(S32 max_len, bool report_timeout, bool from_board, S32 *actual_len) { return (C8*)""; }
    S32 WINAPI GPIB_set_EOS_mode(S32 new_EOS_char, bool send_EOI_at_EOT, bool enable_board_configuration) { return 0; }
    C8* WINAPI GPIB_query(C8 *string, bool from_board) { return (C8*)""; }
    void __cdecl SA_query_printf(C8 *fmt, ...) {}
    void WINAPI SAL_alert_box(C8 *title, C8 *msg) {}
    SA_STATE* WINAPI SA_startup() { return NULL; }
    void WINAPI SA_parse_command_line(C8 *command_line) {}
    bool WINAPI SA_connect(S32 device_address, C8 *command_line, GPIBERR handler) { return true; }
    void WINAPI SA_fetch_trace() {}
    void WINAPI SA_shutdown() {}
    void WINAPI SA_resample_data(DOUBLE *src, S32 ns, DOUBLE *dest, S32 nd, RESAMPLE_OP operation) {}
}

class MockGPIB : public IGPIBInterface {
public:
    MOCK_METHOD(void, Connect, (S32, S32, S32, bool), (override));
    MOCK_METHOD(void, SetEOSMode, (S32, bool, bool), (override));
    MOCK_METHOD(void, SetSerialReadDropout, (S32), (override));
    MOCK_METHOD(void, Print, (const char*), (override));
    MOCK_METHOD(const char*, ReadASC, (S32, bool), (override));
    MOCK_METHOD(bool, IsAborted, (), (override));
    MOCK_METHOD(void, SetAbort, (bool), (override));
};

class MockUI : public IPlotterUI {
public:
    MOCK_METHOD(void, Alert, (const char*, const char*), (override));
    MOCK_METHOD(void, ServeMessageQueue, (), (override));
    MOCK_METHOD(void, UpdateStatus, (const char*, S32), (override));
    MOCK_METHOD(void, Refresh, (), (override));
    MOCK_METHOD(void, RenderSourceList, (), (override));
};

class PlotterLogicTest : public ::testing::Test {
protected:
    MockGPIB gpib;
    MockUI ui;
    PlotterConfig config;

    void SetUp() override {
        memset(&config, 0, sizeof(config));
        config.async_timeout = 1000;
        config.async_xfer_size = 1024;
        config.min_plot_bytes = 10;

        GPIB_clear_error();     // the latch is global state -- don't leak it between tests
    }

    void TearDown() override {
        GPIB_clear_error();
    }
};

TEST_F(PlotterLogicTest, SuccessfulAcquisition) {
    EXPECT_CALL(gpib, Connect(_, _, _, _));
    EXPECT_CALL(gpib, SetEOSMode(_, _, _));
    EXPECT_CALL(gpib, SetSerialReadDropout(_));

    {
        InSequence s;
        EXPECT_CALL(gpib, ReadASC(_, _)).WillOnce(Return("PA0,0;PD;PA100,100;"));
        EXPECT_CALL(gpib, ReadASC(_, _)).WillRepeatedly(Return("")); 
    }

    EXPECT_CALL(gpib, IsAborted()).WillRepeatedly(Return(false));

    config.async_timeout = 100; // 100ms for faster test

    PlotterLogic logic(&gpib, &ui, config);
    char* result = logic.AsyncRead(30, TRUE, 1, NULL, FALSE, TRUE);

    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "PA0,0;PD;PA100,100;");
}

TEST_F(PlotterLogicTest, HandlesHPGLQueries) {
    EXPECT_CALL(gpib, Connect(_, _, _, _));
    
    // Simulate OI query in the stream
    {
        InSequence s;
        EXPECT_CALL(gpib, ReadASC(_, _)).WillOnce(Return("OI"));
        EXPECT_CALL(gpib, ReadASC(_, _)).WillOnce(Return("")); // Timeout triggers query processing
        EXPECT_CALL(gpib, Print(_)).WillOnce(Return()); // Should respond to OI
        EXPECT_CALL(gpib, ReadASC(_, _)).WillOnce(Return("PA100,100;"));
        EXPECT_CALL(gpib, ReadASC(_, _)).WillRepeatedly(Return("")); // Final timeout
    }

    EXPECT_CALL(gpib, IsAborted()).WillRepeatedly(Return(false));

    config.async_timeout = 100;

    strcpy(config.OI_reply, "7470A;");

    PlotterLogic logic(&gpib, &ui, config);
    char* result = logic.AsyncRead(30, TRUE, 1, NULL, FALSE, TRUE);

    ASSERT_NE(result, nullptr);
    // Note: The logic removes the "OI" from contents after responding
    EXPECT_STREQ(result, "PA100,100;");
}

TEST_F(PlotterLogicTest, AbortHandling) {
    EXPECT_CALL(gpib, IsAborted()).WillOnce(Return(true));

    PlotterLogic logic(&gpib, &ui, config);
    char* result = logic.AsyncRead(30, TRUE, 1, NULL, FALSE, TRUE);

    EXPECT_EQ(result, nullptr);
}

//
// Non-fatal GPIB error handling.  Before this, GPIB_error() called exit(1),
// so pointing an acquisition at an instrument that couldn't answer took the
// whole program down along with any plot already on screen.
//

TEST_F(PlotterLogicTest, ErrorLatchKeepsFirstMessage) {
    EXPECT_EQ(GPIB_error_pending, 0);

    GPIB_error((C8*)"viRead_ASC Error: 0xBFFF0015", 0, 0, 0);
    GPIB_error((C8*)"a later error caused by the first one", 0, 0, 0);

    EXPECT_EQ(GPIB_error_pending, 1);
    EXPECT_STREQ(GPIB_error_text, "viRead_ASC Error: 0xBFFF0015");

    GPIB_clear_error();

    EXPECT_EQ(GPIB_error_pending, 0);
    EXPECT_STREQ(GPIB_error_text, "");
}

TEST_F(PlotterLogicTest, ErrorLatchToleratesNullMessage) {
    GPIB_error(NULL, 0, 0, 0);

    EXPECT_EQ(GPIB_error_pending, 1);
    EXPECT_GT(strlen(GPIB_error_text), 0u);
}

TEST_F(PlotterLogicTest, FailedConnectStopsPollingImmediately) {
    // A connect that fails latches an error, and there's no point waiting
    // around for data that will never arrive
    EXPECT_CALL(gpib, Connect(_, _, _, _))
        .WillOnce(::testing::InvokeWithoutArgs(
            []() { GPIB_error((C8*)"Could not open VISA resource", 0, 0, 0); }));

    EXPECT_CALL(gpib, IsAborted()).WillRepeatedly(Return(false));
    EXPECT_CALL(gpib, ReadASC(_, _)).Times(0);

    PlotterLogic logic(&gpib, &ui, config);
    char* result = logic.AsyncRead(30, TRUE, 1, NULL, FALSE, TRUE);

    EXPECT_EQ(result, nullptr);
    EXPECT_EQ(GPIB_error_pending, 1);
}

TEST_F(PlotterLogicTest, ErrorDiscardsPartiallyReceivedPlot) {
    // Enough bytes arrive to satisfy min_plot_bytes, then the read fails.  The
    // partial plot must be thrown away rather than rendered as a truncated
    // trace over the good one the user already had
    EXPECT_CALL(gpib, Connect(_, _, _, _));

    {
        InSequence s;
        EXPECT_CALL(gpib, ReadASC(_, _)).WillOnce(Return("PA0,0;PD;PA100,100;"));
        EXPECT_CALL(gpib, ReadASC(_, _)).WillRepeatedly(::testing::InvokeWithoutArgs([]() {
            GPIB_error((C8*)"viRead_ASC Error: 0xBFFF0015", 0, 0, 0);
            return (const char*)"";
        }));
    }

    EXPECT_CALL(gpib, IsAborted()).WillRepeatedly(Return(false));

    config.async_timeout = 100;

    PlotterLogic logic(&gpib, &ui, config);
    char* result = logic.AsyncRead(30, TRUE, 1, NULL, FALSE, TRUE);

    EXPECT_EQ(result, nullptr);
    EXPECT_EQ(GPIB_error_pending, 1);
}
