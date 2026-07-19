#ifndef _7470_LOGIC_H
#define _7470_LOGIC_H

#include "typedefs.h"

//
// Non-fatal GPIB error latch
//
// GPIBLIB invokes the GPIBERR handler from inside a failed vi/ib read or
// write, and every one of those call sites returns NULL or 0 to its caller as
// soon as the handler returns.  GPIB_error() takes advantage of that: instead
// of terminating the program (which would throw away the plot that's currently
// on screen because of one mistyped address or misclicked acquisition
// shortcut), it records the first error of the operation and lets the failed
// read unwind normally.
//
// Acquisition code arms the latch with GPIB_clear_error(), then treats a set
// GPIB_error_pending as "discard whatever partial data was received and leave
// the previous trace alone".  Reporting the error to the user is the
// application's business -- see GPIB_report_error() in 7470.cpp.
//

extern S32 GPIB_error_pending;
extern C8  GPIB_error_text[2048];

void WINAPI GPIB_error       (C8 *msg, S32 ibsta, S32 iberr, S32 ibcntl);
void        GPIB_clear_error (void);

class IGPIBInterface {
public:
    virtual ~IGPIBInterface() {}
    virtual void Connect(S32 device_address, S32 timeout_ms, S32 plotter_address, bool release_sys_control) = 0;
    virtual void SetEOSMode(S32 eos_char, bool enable_read, bool enable_write) = 0;
    virtual void SetSerialReadDropout(S32 dropout_ms) = 0;
    virtual void Print(const char* msg) = 0;
    virtual const char* ReadASC(S32 size, bool inhibit_error_reports) = 0;
    virtual bool IsAborted() = 0;
    virtual void SetAbort(bool abort) = 0;
};

class IPlotterUI {
public:
    virtual ~IPlotterUI() {}
    virtual void Alert(const char* title, const char* msg) = 0;
    virtual void ServeMessageQueue() = 0;
    virtual void UpdateStatus(const char* text, S32 total_len) = 0;
    virtual void Refresh() = 0;
    virtual void RenderSourceList() = 0;
};

struct PlotterConfig {
    S32 async_GPIB_timeout_ms;
    S32 async_timeout;
    S32 async_xfer_size;
    S32 serial_read_dropout;
    S32 min_plot_bytes;
    bool release_sys_control;
    bool ignore_write_aborts;
    bool process_OS;
    bool SP0_detect;
    char HPGL_preface[256];
    char OE_reply[256];
    char OH_reply[256];
    char OI_reply[256];
    char OP_reply[256];
    char OO_reply[256];
    char OF_reply[256];
    char OA_reply[256];
    char OC_reply[256];
};

class PlotterLogic {
public:
    PlotterLogic(IGPIBInterface* gpib, IPlotterUI* ui, const PlotterConfig& config);
    
    char* AsyncRead(S32 plotter_address,
                    bool refresh_display,
                    S32 device_address,
                    const char* device_command,
                    bool allow_source_list_refresh,
                    bool reset_to_local);

private:
    IGPIBInterface* m_gpib;
    IPlotterUI* m_ui;
    PlotterConfig m_config;
    static char m_contents[4194304];
};

#endif
