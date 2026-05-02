#include <gtest/gtest.h>
#include "../src/apps/satrace_utils.h"
#include <windows.h>

TEST(SatraceUtils, AscNum) {
    EXPECT_EQ(ascnum((C8*)"123", 10), 123);
    EXPECT_EQ(ascnum((C8*)"-456", 10), -456);
    EXPECT_EQ(ascnum((C8*)"FF", 16), 255);
}

TEST(SatraceUtils, ParseOptionsSimple) {
    SatraceOptions opt;
    // Simulate "satrace 18 -reps:5"
    // Note: parse_satrace_options expects the FULL command line including the exe name/path
    parse_satrace_options(opt, (C8*)"satrace.exe 18 -reps:5");
    
    EXPECT_EQ(opt.reps, 5);
    EXPECT_STREQ(opt.lpCmdLine, "18");
}

TEST(SatraceUtils, ParseOptionsQuotedPath) {
    SatraceOptions opt;
    // Simulate "\"C:\\Program Files\\satrace.exe\" 18 -lf -header"
    parse_satrace_options(opt, (C8*)"\"C:\\Program Files\\satrace.exe\" 18 -lf -header");
    
    EXPECT_TRUE(opt.LF_separator);
    EXPECT_TRUE(opt.show_header);
    EXPECT_STREQ(opt.lpCmdLine, "18");
}

TEST(SatraceUtils, ParseOptionsComplex) {
    SatraceOptions opt;
    parse_satrace_options(opt, (C8*)"satrace 18 -spline:800 -avg:128");
    
    // The parser loop processes options in a specific order. Spline is checked last
    // in the if-sequence, so it wins if multiple resampling options are present
    // because it will be processed in a later iteration or later in the loop.
    EXPECT_EQ(opt.n_dest_pts, 800);
    EXPECT_EQ(opt.resamp_op, RT_SPLINE);
    EXPECT_STREQ(opt.lpCmdLine, "18");
}

class MockOutput : public ITraceOutput {
public:
    std::string output;
    void Print(const char* format, ...) override {
        char buf[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        output += buf;
    }
};

TEST(TraceProcessor, CalculateBinFrequency) {
    // 10MHz to 20MHz, 10 points
    // df = 1MHz. first bin center at 10.5MHz
    EXPECT_NEAR(TraceProcessor::CalculateBinFrequency(10.0, 20.0, 10, 0), 10.5, 0.001);
    EXPECT_NEAR(TraceProcessor::CalculateBinFrequency(10.0, 20.0, 10, 1), 11.5, 0.001);
    EXPECT_NEAR(TraceProcessor::CalculateBinFrequency(10.0, 20.0, 10, 9), 19.5, 0.001);
}

TEST(TraceProcessor, PrintHeader) {
    MockOutput mock;
    SA_STATE state;
    memset(&state, 0, sizeof(state));
    strcpy(state.ID_string, "MOCK_SA");
    state.min_Hz = 100.0;
    state.max_Hz = 200.0;
    state.n_trace_points = 5;
    state.RBW_Hz = -1.0; // Invalid
    state.VBW_Hz = 10.0;

    TraceProcessor::PrintHeader(mock, &state);
    EXPECT_TRUE(mock.output.find("instrument_model MOCK_SA") != std::string::npos);
    EXPECT_TRUE(mock.output.find("start_freq_Hz 100.000000") != std::string::npos);
    EXPECT_TRUE(mock.output.find("VBW_Hz 10.000000") != std::string::npos);
    EXPECT_TRUE(mock.output.find("RBW_Hz") == std::string::npos);
}

TEST(TraceProcessor, PrintTraceData) {
    MockOutput mock;
    SA_STATE state;
    memset(&state, 0, sizeof(state));
    state.min_Hz = 0.0;
    state.max_Hz = 100.0;
    DOUBLE values[] = { -10.0, -20.0 };
    
    TraceProcessor::PrintTraceData(mock, &state, values, 2, false);
    // df = 50. bins at 25 and 75
    EXPECT_STREQ(mock.output.c_str(), "trace_data 25.000000,-10.000000, 75.000000,-20.000000");
}
