#include "winvfx.h"
#include "xy.h"
#include <gtest/gtest.h>

TEST(XYTest, Init) {
    // XY needs a PANE
    PANE p;
    p.x0 = 0; p.y0 = 0; p.x1 = 100; p.y1 = 100;
    
    XY xy(&p, &p);
    // Basic check that we can instantiate
}
