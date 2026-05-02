#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include "satrace_utils.h"
#include "gpib_utils.h"

void parse_satrace_options(SatraceOptions &opt, const C8 *raw_cmdline) {
    opt.reps = 1;
    opt.n_dest_pts = -1;
    opt.resamp_op = RT_POINT;
    opt.LF_separator = false;
    opt.show_header = false;
    memset(opt.lpCmdLine, 0, sizeof(opt.lpCmdLine));

    // Handle initial skipping of the executable path in the raw_cmdline
    const C8 *cmdtail = raw_cmdline;
    if (cmdtail != NULL) {
        if (*cmdtail == '"') {
            cmdtail = strchr(cmdtail + 1, '"');
            if (cmdtail != NULL) cmdtail++;
        } else {
            cmdtail = strchr(cmdtail, ' ');
        }
        if (cmdtail != NULL) {
            while (*cmdtail == ' ') cmdtail++;
            strcpy(opt.lpCmdLine, cmdtail);
        }
    }

    C8 *option = NULL;
    C8 *end = NULL;

    while (1) {
        // Remove leading/trailing spaces
        for (U32 i = 0; i < strlen(opt.lpCmdLine); i++) {
            if (!isspace(opt.lpCmdLine[i])) {
                memmove(opt.lpCmdLine, &opt.lpCmdLine[i], strlen(&opt.lpCmdLine[i]) + 1);
                break;
            }
        }
        C8 *p = &opt.lpCmdLine[strlen(opt.lpCmdLine) - 1];
        while (p >= opt.lpCmdLine) {
            if (!isspace(*p)) break;
            *p-- = 0;
        }

        if ((option = strstr(opt.lpCmdLine, "-reps:")) != NULL) {
            opt.reps = (S32)ascnum(&option[6], 10, &end);
            memmove(option, end, strlen(end) + 1);
            continue;
        }
        if ((option = strstr(opt.lpCmdLine, "-lf")) != NULL) {
            opt.LF_separator = true;
            memmove(option, &option[3], strlen(&option[3]) + 1);
            continue;
        }
        if ((option = strstr(opt.lpCmdLine, "-header")) != NULL) {
            opt.show_header = true;
            memmove(option, &option[7], strlen(&option[7]) + 1);
            continue;
        }
        if ((option = strstr(opt.lpCmdLine, "-min:")) != NULL) {
            opt.n_dest_pts = (S32)ascnum(&option[5], 10, &end);
            opt.resamp_op = RT_MIN;
            memmove(option, end, strlen(end) + 1);
            continue;
        }
        if ((option = strstr(opt.lpCmdLine, "-max:")) != NULL) {
            opt.n_dest_pts = (S32)ascnum(&option[5], 10, &end);
            opt.resamp_op = RT_MAX;
            memmove(option, end, strlen(end) + 1);
            continue;
        }
        if ((option = strstr(opt.lpCmdLine, "-avg:")) != NULL) {
            opt.n_dest_pts = (S32)ascnum(&option[5], 10, &end);
            opt.resamp_op = RT_AVG;
            memmove(option, end, strlen(end) + 1);
            continue;
        }
        if ((option = strstr(opt.lpCmdLine, "-point:")) != NULL) {
            opt.n_dest_pts = (S32)ascnum(&option[7], 10, &end);
            opt.resamp_op = RT_POINT;
            memmove(option, end, strlen(end) + 1);
            continue;
        }
        if ((option = strstr(opt.lpCmdLine, "-spline:")) != NULL) {
            opt.n_dest_pts = (S32)ascnum(&option[8], 10, &end);
            opt.resamp_op = RT_SPLINE;
            memmove(option, end, strlen(end) + 1);
            continue;
        }
        break;
    }
}

void TraceProcessor::PrintHeader(ITraceOutput& out, const SA_STATE* state) {
    out.Print("instrument_model %s\n", state->ID_string);
    out.Print("start_freq_Hz %lf\n", state->min_Hz);
    out.Print("stop_freq_Hz %lf\n", state->max_Hz);
    out.Print("source_trace_points %d\n", state->n_trace_points);
    out.Print("reference_level_dBm %lf\n", state->max_dBm);
    out.Print("n_vertical_divisions %d\n", state->amplitude_levels);
    out.Print("dB_per_division %d\n", state->dB_division);
    if (state->RBW_Hz >= 0.0F) out.Print("RBW_Hz %lf\n", state->RBW_Hz);
    if (state->VBW_Hz >= 0.0F) out.Print("VBW_Hz %lf\n", state->VBW_Hz);
    if (state->vid_avgs >= 0) out.Print("vid_avgs %d\n", state->vid_avgs);
    if (state->sweep_secs >= 0.0F) out.Print("sweep_secs %lf\n", state->sweep_secs);
    if (state->RFATT_dB > -10000.0F) out.Print("RF_atten_dB %lf\n", state->RFATT_dB);
}

void TraceProcessor::PrintTraceData(ITraceOutput& out, const SA_STATE* state, const DOUBLE* values, S32 n_pts, bool lf_separator) {
    out.Print(lf_separator ? "trace_data\n" : "trace_data ");

    DOUBLE df = (state->max_Hz - state->min_Hz) / (DOUBLE)n_pts;
    for (S32 i = 0; i < n_pts; i++) {
        DOUBLE f = CalculateBinFrequency(state->min_Hz, state->max_Hz, n_pts, i);
        out.Print("%lf,%lf", f, values[i]);

        if (i != n_pts - 1) {
            out.Print(lf_separator ? "\n" : ", ");
        }
    }
}

DOUBLE TraceProcessor::CalculateBinFrequency(DOUBLE min_Hz, DOUBLE max_Hz, S32 n_pts, S32 bin_index) {
    DOUBLE df = (max_Hz - min_Hz) / (DOUBLE)n_pts;
    return min_Hz + (df / 2.0) + (bin_index * df);
}
