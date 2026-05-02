#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define BUILD_VFX
#include "renderer.h"
#include <vector>
#include <iostream>

// Forward declaration of parse_hpgl since it's not in renderer.h
void parse_hpgl(U8 data);

// Define externs required by renderer.cpp
S32 RES_X = 800;
S32 RES_Y = 600;
PANE *stage = NULL;
S32 pen_colors[256];
S32 standard_pen_colors[256];
S32 background = 0;
C8 antialias = 0;
U8 in_hpgl = 1;
U8 force_lb_term = 0;
int user_rotate = 0;
int INI_background = 0;
U8 plotter_mode = 0;
double GL_XMIN = 0, GL_XMAX = 10000, GL_YMIN = 0, GL_YMAX = 10000;
double gl_ip_xmin = 0, gl_ip_xmax = 10000, gl_ip_ymin = 0, gl_ip_ymax = 10000;
double gl_sc_xmin = 0, gl_sc_xmax = 10000, gl_sc_ymin = 0, gl_sc_ymax = 10000;
U8 gl_clip = 0;
double gl_clip_x1 = 0, gl_clip_y1 = 0, gl_clip_x2 = 10000, gl_clip_y2 = 10000;
int sc_enabled = 0;

// Mock VFX functions
struct PixelOp {
    int x, y;
    U32 color;
};
std::vector<PixelOp> pixel_log;

extern "C" {
    U32 WINAPI VFX_pixel_write(PANE *pane, S32 x, S32 y, U32 color) {
        pixel_log.push_back({x, y, color});
        return 0;
    }
    
    VFX_RGB * WINAPI VFX_color_to_RGB(U32 color) {
        static VFX_RGB rgb = {255, 255, 255};
        return &rgb;
    }
    
    void WINAPI VFX_Cos_Sin(S32 angle, F16 *cos, F16 *sin) {
        *cos = 65536; // 1.0
        *sin = 0;     // 0.0
    }
}

class HPGLRenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        render_init();
        pixel_log.clear();
        viewport_init();
        gl_set_defaults(2);
        
        for(int i=0; i<256; i++) pen_colors[i] = i;
        for(int i=0; i<256; i++) standard_pen_colors[i] = i;
    }
    
    void send_hpgl(const char* data) {
        while (*data) {
            parse_hpgl((U8)*data++);
        }
    }
};

TEST_F(HPGLRenderTest, MoveAbsolute) {
    send_hpgl("PA500,500;");
    EXPECT_DOUBLE_EQ(gl_x, 500.0);
    EXPECT_DOUBLE_EQ(gl_y, 500.0);
}

TEST_F(HPGLRenderTest, DrawLine) {
    send_hpgl("PA100,100;PD;PA200,200;");
    EXPECT_DOUBLE_EQ(gl_x, 200.0);
    EXPECT_DOUBLE_EQ(gl_y, 200.0);
    EXPECT_EQ(gl_pendown, 1);
    EXPECT_FALSE(pixel_log.empty());
}

TEST_F(HPGLRenderTest, PenUp) {
    send_hpgl("PD;PU;");
    EXPECT_EQ(gl_pendown, 0);
}

TEST_F(HPGLRenderTest, Scaling) {
    send_hpgl("SC0,10,0,10;PA5,5;");
    EXPECT_DOUBLE_EQ(gl_x, 5.0);
    EXPECT_DOUBLE_EQ(gl_y, 5.0);
}

TEST_F(HPGLRenderTest, MultiplePairs) {
    send_hpgl("PA0,0,100,100,200,200;");
    EXPECT_DOUBLE_EQ(gl_x, 200.0);
    EXPECT_DOUBLE_EQ(gl_y, 200.0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
