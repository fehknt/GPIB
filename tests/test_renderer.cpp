#include "renderer.h"
#include <gtest/gtest.h>

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
double GL_XMIN = 0, GL_XMAX = 1000, GL_YMIN = 0, GL_YMAX = 1000;
double gl_ip_xmin = 0, gl_ip_xmax = 1000, gl_ip_ymin = 0, gl_ip_ymax = 1000;
double gl_sc_xmin = 0, gl_sc_xmax = 1000, gl_sc_ymin = 0, gl_sc_ymax = 1000;
U8 gl_clip = 0;
double gl_clip_x1 = 0, gl_clip_y1 = 0, gl_clip_x2 = 1000, gl_clip_y2 = 1000;
int sc_enabled = 0;

TEST(Renderer, ViewportInit) {
    viewport_init();
    ASSERT_EQ(pVP->WinX1, (RES_X / 40));
    ASSERT_EQ(pVP->WinY1, (RES_Y / 24));
}

TEST(Renderer, Scaling) {
    viewport_init();
    // Default scaling: world 0..1000 maps to screen
    int sx = scale_x(0.0);
    // Should be around WinX1
    ASSERT_TRUE(sx >= pVP->WinX1);
}
