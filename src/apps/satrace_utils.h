#ifndef SATRACE_UTILS_H
#define SATRACE_UTILS_H

#include "typedefs.h"
#include "specan.h"
#include "gpib_utils.h"

#define VERSION "1.12"

struct SatraceOptions {
    S32 reps;
    S32 n_dest_pts;
    RESAMPLE_OP resamp_op;
    bool LF_separator;
    bool show_header;
    C8 lpCmdLine[MAX_PATH];
};

void parse_satrace_options(SatraceOptions &opt, const C8 *raw_cmdline);

class ITraceOutput {
public:
    virtual ~ITraceOutput() {}
    virtual void Print(const char* format, ...) = 0;
};

class StdoutOutput : public ITraceOutput {
public:
    void Print(const char* format, ...) override {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
};

class TraceProcessor {
public:
    static void PrintHeader(ITraceOutput& out, const SA_STATE* state);
    static void PrintTraceData(ITraceOutput& out, const SA_STATE* state, const DOUBLE* values, S32 n_pts, bool lf_separator);
    static DOUBLE CalculateBinFrequency(DOUBLE min_Hz, DOUBLE max_Hz, S32 n_pts, S32 bin_index);
};

#endif
