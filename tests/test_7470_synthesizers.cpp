#include <gtest/gtest.h>
#include <gmock/gmock.h>

// BUILDING_GPIBLIB / BUILDING_SPECAN come from gpib_common's PUBLIC compile
// definitions; defining them here too just produces C4005

#include "typedefs.h"
#include "gpiblib.h"
#include "specan.h"
#include "7470_synthesizers.h"
#include <string.h>

// Mocking GPIB and SA functions
void __cdecl GPIB_print(C8 *fmt, ...) {}

extern "C" {
    C8* WINAPI GPIB_read_ASC(S32 max_len, bool report_timeout, bool from_board) {
        static C8 buf[8192];
        memset(buf, 0, sizeof(buf));
        // For SAVEA?
        if (max_len == -1) return (C8*)"SAVEA OFF";
        
        // For OT query, return 32 strings separated by CR/LF
        C8* p = buf;
        for(int i=0; i<32; i++) {
            p += sprintf(p, "Annotation %d\r\n", i+1);
        }
        return buf;
    }

    C8* WINAPI GPIB_read_BIN(S32 max_len, bool report_timeout, bool from_board, S32 *actual_len) {
        static C8 bin_buf[8192];
        memset(bin_buf, 0, sizeof(bin_buf));
        if (actual_len) *actual_len = max_len;
        return bin_buf;
    }

    S32 WINAPI GPIB_set_EOS_mode(S32 new_EOS_char, bool send_EOI_at_EOT, bool enable_board_configuration) {
        return 0;
    }

    C8* WINAPI GPIB_query(C8 *string, bool from_board) {
        static C8 q_buf[1024];
        strcpy(q_buf, "OFF");
        return q_buf;
    }
    
    void __cdecl SA_query_printf(C8 *fmt, ...) {}
    
    SA_STATE* WINAPI SA_startup(void) {
        static SA_STATE sa;
        memset(&sa, 0, sizeof(sa));
        sa.n_trace_points = 1000;
        sa.dBm_values = (DOUBLE*)malloc(1000 * sizeof(DOUBLE));
        for(int i=0; i<1000; i++) sa.dBm_values[i] = -50.0;
        return &sa;
    }
    
    void WINAPI SA_parse_command_line(C8 *command_line) {}
    
    bool WINAPI SA_connect(S32 device_address, C8 *command_line, GPIBERR handler) {
        return true;
    }
    
    void WINAPI SA_fetch_trace(void) {}
    
    void WINAPI SA_shutdown(void) {}
    
    void WINAPI SA_resample_data(DOUBLE *src, S32 ns, DOUBLE *dest, S32 nd, RESAMPLE_OP operation) {
        for(int i=0; i<nd; i++) dest[i] = -50.0;
    }

    S32 INI_ignore_write_aborts = 0;
    void WINAPI SAL_alert_box(C8 *title, C8 *msg) {}
}

TEST(Synthesizers, Tek492P) {
    C8* result = synthesize_492P(10);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(strstr(result, "SP3;") != nullptr);
}

TEST(Synthesizers, HP856xA) {
    C8* result = synthesize_856xA(10, false);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(strstr(result, "SP3;") != nullptr);
}

TEST(Synthesizers, HP3585A) {
    C8* result = synthesize_3585A(10);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(strstr(result, "SP3;") != nullptr);
}

TEST(Synthesizers, GenericSA) {
    C8* result = synthesize_generic_SA(10, false);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(strstr(result, "SP3;") != nullptr);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
