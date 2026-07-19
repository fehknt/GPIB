#include "7470_logic.h"
#include "7470_synthesizers.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <windows.h>
#include <mmsystem.h>

char PlotterLogic::m_contents[4194304];

//
// Non-fatal GPIB error latch -- see the comment in 7470_logic.h
//

S32 GPIB_error_pending = 0;
C8  GPIB_error_text[2048] = "";

void WINAPI GPIB_error(C8 *msg, S32 ibsta, S32 iberr, S32 ibcntl) {
    if (GPIB_error_pending) {
        return;             // keep the first error; the ones that follow are usually fallout
    }

    GPIB_error_pending = 1;

    _snprintf(GPIB_error_text,
              sizeof(GPIB_error_text) - 1,
              "%s",
              (msg != NULL) ? msg : "Unspecified GPIB error");

    GPIB_error_text[sizeof(GPIB_error_text) - 1] = 0;
}

void GPIB_clear_error(void) {
    GPIB_error_pending = 0;
    GPIB_error_text[0] = 0;
}

PlotterLogic::PlotterLogic(IGPIBInterface* gpib, IPlotterUI* ui, const PlotterConfig& config)
    : m_gpib(gpib), m_ui(ui), m_config(config) {
}

char* PlotterLogic::AsyncRead(S32 plotter_address,
                             bool refresh_display,
                             S32 device_address,
                             const char* device_command,
                             bool allow_source_list_refresh,
                             bool reset_to_local) {
    if (device_command != NULL) {
        if (!_stricmp(device_command, "HP 70000")) {
            return synthesize_generic_SA(device_address, FALSE);
        }
        if (!_stricmp(device_command, "SCPI")) {
            return synthesize_generic_SA(device_address, TRUE);
        }
        if ((!_stricmp(device_command, "Tektronix 492P")) ||
            (!_stricmp(device_command, "Tektronix 496P"))) {
            return synthesize_492P(device_address);
        }
    }

    m_gpib->Connect(device_address,
                   m_config.async_GPIB_timeout_ms,
                   plotter_address,
                   m_config.release_sys_control);

    m_gpib->SetEOSMode(10, TRUE, FALSE);
    m_gpib->SetSerialReadDropout(m_config.serial_read_dropout);

    bool read_complete = FALSE;
    S32 last_incoming_time = 0;
    S32 last_refresh_time = 0;
    S32 total_len = 0;
    S32 OS_status = 24;

    m_contents[0] = 0;
    if (m_config.HPGL_preface[0]) {
        strcpy(m_contents, m_config.HPGL_preface);
    }

    S32 start_data = (S32)strlen(m_contents);

    if (device_command != NULL) {
        if (!_stricmp(device_command, "HP 856xA")) {
            char *result = synthesize_856xA(device_address, FALSE);
            if (result != NULL) {
                strcat(m_contents, result);
                read_complete = TRUE;
            }
            goto GPIB_done;
        } else if (!_stricmp(device_command, "HP 3585")) {
            char *result = synthesize_3585A(device_address);
            if (result != NULL) {
                strcat(m_contents, result);
                read_complete = TRUE;
            }
            goto GPIB_done;
        } else if (!_stricmp(device_command, "Tektronix 278x")) {
            char *result = synthesize_856xA(device_address, TRUE);
            if (result != NULL) {
                strcat(m_contents, result);
                read_complete = TRUE;
            }
            goto GPIB_done;
        } else {
            m_gpib->Print(device_command);
        }
    }

GPIB_receive_plot:
    m_contents[start_data] = 0;
    last_incoming_time = 0;
    last_refresh_time = 0;
    total_len = 0;
    OS_status = 24;

GPIB_wait:
    while (1) {
        Sleep(5);
        S32 cur_time = (S32)timeGetTime();

        m_ui->ServeMessageQueue();

        // A latched GPIB error means the connect or a write failed outright;
        // there's no point polling for data that will never arrive
        if ((m_gpib->IsAborted()) || (GPIB_error_pending)) {
            goto GPIB_done;
        }

        if ((refresh_display) &&
            ((last_refresh_time == 0) || (cur_time - last_refresh_time > 1000))) {
            m_ui->UpdateStatus(NULL, total_len);
            last_refresh_time = cur_time;
            m_ui->Refresh();
        }

        if ((last_incoming_time != 0) &&
            (cur_time - last_incoming_time > m_config.async_timeout)) {
            read_complete = TRUE;
            goto GPIB_plot_received;
        }

        const char *read_result = m_gpib->ReadASC(m_config.async_xfer_size, TRUE);

        if ((read_result == NULL) || (read_result[0] == 0)) {
            S32 len = (S32)strlen(m_contents);
            S32 end = len - 1;

            while ((end >= 0) && (isspace((U8)m_contents[end]) || (m_contents[end] == ';'))) {
                end--;
            }

            if (end >= 1) {
                bool replied = FALSE;
                char *tail = &m_contents[end - 1];

                if (m_config.OE_reply[0] && (!_strnicmp(tail, "OE", 2))) { m_gpib->Print(m_config.OE_reply); replied = TRUE; }
                if (m_config.OH_reply[0] && (!_strnicmp(tail, "OH", 2))) { m_gpib->Print(m_config.OH_reply); replied = TRUE; }
                if (m_config.OI_reply[0] && (!_strnicmp(tail, "OI", 2))) { m_gpib->Print(m_config.OI_reply); replied = TRUE; }
                if (m_config.OP_reply[0] && (!_strnicmp(tail, "OP", 2))) { m_gpib->Print(m_config.OP_reply); replied = TRUE; }
                if (m_config.OO_reply[0] && (!_strnicmp(tail, "OO", 2))) { m_gpib->Print(m_config.OO_reply); replied = TRUE; }
                if (m_config.OF_reply[0] && (!_strnicmp(tail, "OF", 2))) { m_gpib->Print(m_config.OF_reply); replied = TRUE; }
                if (m_config.OA_reply[0] && (!_strnicmp(tail, "OA", 2))) { m_gpib->Print(m_config.OA_reply); replied = TRUE; }
                if (m_config.OC_reply[0] && (!_strnicmp(tail, "OC", 2))) { m_gpib->Print(m_config.OC_reply); replied = TRUE; }

                if (m_config.process_OS && (!_strnicmp(tail, "OS", 2))) {
                    char buf[32];
                    sprintf(buf, "%d", OS_status);
                    m_gpib->Print(buf);
                    OS_status = 16;
                    replied = TRUE;
                }

                if (replied) {
                    *tail = 0;
                    len = (S32)strlen(m_contents);
                }

                if ((m_config.SP0_detect) && (!replied) && (len > m_config.async_xfer_size)) {
                    char *last_chunk = &m_contents[len - m_config.async_xfer_size];
                    if ((strstr(last_chunk, "SP;") != NULL) ||
                        (strstr(last_chunk, "SP0;") != NULL)) {
                        read_complete = TRUE;
                        goto GPIB_plot_received;
                    }
                }
            }
            goto GPIB_wait;
        }

        S32 read_len = (S32)strlen(read_result);
        if (!read_len) continue;
        if ((U8)read_result[0] == 0xFF) continue;

        total_len += read_len;
        strcat(m_contents, read_result);

        if (last_incoming_time == 0) last_refresh_time = 0;
        last_incoming_time = cur_time;
    }

GPIB_plot_received:
    if (strlen(m_contents) < (U32)m_config.min_plot_bytes) {
        read_complete = FALSE;
        goto GPIB_receive_plot;
    }

GPIB_done:
    if (GPIB_error_pending) return NULL;   // discard whatever partial plot we collected
    if (read_complete) return m_contents;
    return NULL;
}
