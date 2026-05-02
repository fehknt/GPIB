#include "gfxfile.h"
#include <gtest/gtest.h>

TEST(GfxFile, CQBasic) {
    CQ quantizer;
    quantizer.reset();
    
    VFX_RGB red = { 255, 0, 0 };
    VFX_RGB green = { 0, 255, 0 };
    VFX_RGB blue = { 0, 0, 255 };
    
    quantizer.add_color(&red);
    quantizer.add_color(&green);
    quantizer.add_color(&blue);
    
    VFX_RGB palette[16];
    U32 count = quantizer.quantize(palette, 16);
    
    ASSERT_TRUE(count > 0);
}
