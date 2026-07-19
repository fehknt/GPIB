#include "winvfx.h"
#include "xy.h"
#include <gtest/gtest.h>

//
// XY drives a real-time X/Y display.  Its drawing methods (draw_graph,
// draw_scale, draw_cursors, ...) render into a WinVFX PANE and need a display
// surface, so they are not exercised here.  set_input_width(), on the other
// hand, is pure arithmetic: it decides whether the requested point count is
// compatible with the destination pane width, and the class contract says it
// returns 0 when the two aren't evenly divisible.  That's worth pinning down.
//

static XY *make_xy(S32 width)
{
    // The constructor only reads x0/y0/x1/y1 and stores the pointers; it never
    // dereferences pane->window, so a stack PANE is enough to test the sizing
    // logic without a display.
    static PANE graph, outer;

    graph.window = NULL;
    graph.x0 = 0; graph.y0 = 0;
    graph.x1 = width - 1; graph.y1 = 99;

    outer = graph;

    return new XY(&graph, &outer);
}

TEST(XYTest, Construct) {
    XY *xy = make_xy(100);
    ASSERT_NE(xy, nullptr);
    delete xy;
}

TEST(XYTest, SetInputWidthUnity) {
    XY *xy = make_xy(100);
    EXPECT_NE(xy->set_input_width(100), 0);   // exactly the pane width
    delete xy;
}

TEST(XYTest, SetInputWidthEvenOversampleAndUndersample) {
    XY *xy = make_xy(100);
    EXPECT_NE(xy->set_input_width(50),  0);    // pane width 100 divisible by 50
    EXPECT_NE(xy->set_input_width(200), 0);    // 200 divisible by pane width 100
    EXPECT_NE(xy->set_input_width(25),  0);    // 100 divisible by 25
    delete xy;
}

TEST(XYTest, SetInputWidthRejectsIndivisible) {
    XY *xy = make_xy(100);
    EXPECT_EQ(xy->set_input_width(7),   0);    // 100 not a multiple of 7
    EXPECT_EQ(xy->set_input_width(150), 0);    // 150 not a multiple of 100
    EXPECT_EQ(xy->set_input_width(0),   0);    // degenerate request
    delete xy;
}
