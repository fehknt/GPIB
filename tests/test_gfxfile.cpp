#include "winvfx.h"
#include "gfxfile.h"
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

//
// The image writers (TGA/GIF/BMP/PCX/PNG) take a WinVFX PANE, which is why
// they went untested for so long -- a PANE normally comes from an initialized
// display.  We sidestep that here by handing the writers a PANE backed by a
// plain heap buffer with a hand-filled 5-5-5 VFX_WINDOW descriptor, so no
// VFX_set_display_mode / display surface is needed.  (VFX_window_construct
// can't be used headless: it copies the *current* display format, and with no
// display it faults.)
//
// What this does and does NOT verify:
//   - DOES: each writer runs against a real PANE, produces a file, and gets the
//     geometry and container structure right.  BMP is parsed fully; PNG is
//     validated by signature, IHDR fields, and every chunk CRC -- which runs
//     the actual GDI+-backed encoder and my 24bpp packing end to end.
//   - DOES NOT: assert pixel colors.  VFX_RGB_value() expands a native pixel
//     using the *global* display format, which only VFX_set_display_mode()
//     establishes; with no display it returns black, so a color assertion
//     would test the harness's lack of a display, not the writer.  Color
//     fidelity is covered instead by the 7470 hardware/manual path.
//
// GIF/TGA/PCX are LZW/RLE/palettized and there's no decoder in this tree, so
// for those we confirm only that the writer succeeds and emits a real file.
//

#define TEST_W 16
#define TEST_H 16

class GfxWriter : public ::testing::Test {
protected:
    VFX_WINDOW      win;
    PANE            pane;
    unsigned short *buf = NULL;

    void SetUp() override {
        // Off-display 5-5-5 window: a contiguous 16bpp buffer plus the format
        // descriptor the writers read through VFX_pixel_read().
        buf = (unsigned short *)malloc(TEST_W * TEST_H * sizeof(unsigned short));
        ASSERT_NE(buf, nullptr);
        for (int i = 0; i < TEST_W * TEST_H; i++) buf[i] = 0x7C00;   // (color not asserted; see header note)

        memset(&win, 0, sizeof(win));
        win.buffer = buf;
        win.x_max = TEST_W - 1;
        win.y_max = TEST_H - 1;
        win.pixel_pitch = 2;
        win.bytes_per_pixel = 2;
        win.R_left = 10; win.R_right = 3; win.R_mask = 0x7C00; win.R_width = 5;
        win.G_left = 5;  win.G_right = 3; win.G_mask = 0x03E0; win.G_width = 5;
        win.B_left = 0;  win.B_right = 3; win.B_mask = 0x001F; win.B_width = 5;

        pane.window = &win;
        pane.x0 = 0; pane.y0 = 0;
        pane.x1 = TEST_W - 1; pane.y1 = TEST_H - 1;
    }

    void TearDown() override {
        free(buf);
        buf = NULL;
    }

    static long slurp(const char *path, unsigned char *dest, long max) {
        FILE *f = fopen(path, "rb");
        if (f == NULL) return -1;
        long n = (long)fread(dest, 1, max, f);
        fclose(f);
        return n;
    }

    static unsigned long crc32(const unsigned char *p, long n) {
        static unsigned long tbl[256];
        static bool built = false;
        if (!built) {
            for (int i = 0; i < 256; i++) {
                unsigned long c = i;
                for (int k = 0; k < 8; k++)
                    c = (c & 1) ? (0xEDB88320UL ^ (c >> 1)) : (c >> 1);
                tbl[i] = c;
            }
            built = true;
        }
        unsigned long c = 0xFFFFFFFFUL;
        for (long i = 0; i < n; i++)
            c = tbl[(c ^ p[i]) & 0xFF] ^ (c >> 8);
        return c ^ 0xFFFFFFFFUL;
    }

    static unsigned long be32(const unsigned char *p) {
        return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
               ((unsigned long)p[2] << 8)  | (unsigned long)p[3];
    }
};

TEST_F(GfxWriter, BmpGeometry) {
    const char *path = "test_gfx_roundtrip.bmp";
    ASSERT_TRUE(BMP_write_16bpp(&pane, (C8 *)path));

    unsigned char buf[8192];
    long n = slurp(path, buf, sizeof(buf));
    _unlink(path);

    ASSERT_GT(n, 54);                                  // headers + pixels
    EXPECT_EQ(buf[0], 'B');
    EXPECT_EQ(buf[1], 'M');

    // BITMAPINFOHEADER: width/height/bitcount are LE at offsets 18/22/28
    long width    = buf[18] | (buf[19] << 8) | (buf[20] << 16) | (buf[21] << 24);
    long height   = buf[22] | (buf[23] << 8) | (buf[24] << 16) | (buf[25] << 24);
    int  bitcount = buf[28] | (buf[29] << 8);

    EXPECT_EQ(width,  TEST_W);
    EXPECT_EQ(height, TEST_H);
    EXPECT_EQ(bitcount, 24);

    // Pixel array size is consistent with a 24bpp, 4-byte-aligned image
    long row = ((TEST_W * 3) + 3) & ~3;
    long off = buf[10] | (buf[11] << 8) | (buf[12] << 16) | (buf[13] << 24);
    EXPECT_GE(n, off + row * TEST_H);
}

TEST_F(GfxWriter, PngStructureAndCrc) {
    const char *path = "test_gfx_roundtrip.png";
    ASSERT_TRUE(PNG_write_16bpp(&pane, (C8 *)path));    // exercises the GDI+ encoder + 24bpp packing

    unsigned char buf[65536];
    long n = slurp(path, buf, sizeof(buf));
    _unlink(path);

    ASSERT_GT(n, 8);
    static const unsigned char sig[8] = { 0x89,'P','N','G','\r','\n',0x1A,'\n' };
    ASSERT_EQ(memcmp(buf, sig, 8), 0) << "PNG signature missing";

    long off = 8;
    bool seen_ihdr = false, seen_idat = false, seen_iend = false;
    long width = 0, height = 0;
    int depth = 0, ctype = 0;

    while (off + 12 <= n) {
        long len = (long)be32(&buf[off]);
        const unsigned char *type = &buf[off + 4];

        ASSERT_LE(off + 12 + len, n) << "chunk runs past end of file";

        // CRC covers the type field and the chunk data
        unsigned long stored = be32(&buf[off + 8 + len]);
        EXPECT_EQ(stored, crc32(&buf[off + 4], 4 + len))
            << "bad CRC in chunk " << std::string((const char *)type, 4);

        if (!memcmp(type, "IHDR", 4)) {
            seen_ihdr = true;
            width  = (long)be32(&buf[off + 8]);
            height = (long)be32(&buf[off + 12]);
            depth  = buf[off + 16];
            ctype  = buf[off + 17];
        } else if (!memcmp(type, "IDAT", 4)) {
            seen_idat = true;
        } else if (!memcmp(type, "IEND", 4)) {
            seen_iend = true;
        }

        off += 12 + len;
    }

    EXPECT_EQ(off, n) << "trailing bytes after IEND";
    EXPECT_TRUE(seen_ihdr);
    EXPECT_TRUE(seen_idat);
    EXPECT_TRUE(seen_iend);
    EXPECT_EQ(width,  TEST_W);
    EXPECT_EQ(height, TEST_H);
    EXPECT_EQ(depth,  8);
    EXPECT_EQ(ctype,  2);                              // truecolor RGB, no alpha
}

// GIF/TGA/PCX have no decoder in-tree; confirm each writer at least succeeds
// and emits a non-trivial file.
TEST_F(GfxWriter, PalettizedWritersProduceFiles) {
    struct { const char *path; bool (*fn)(PANE *, C8 *); } w[] = {
        { "test_gfx_roundtrip.gif", GIF_write_16bpp },
        { "test_gfx_roundtrip.tga", TGA_write_16bpp },
        { "test_gfx_roundtrip.pcx", PCX_write_16bpp },
    };

    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(w[i].fn(&pane, (C8 *)w[i].path)) << "writer failed: " << w[i].path;

        unsigned char buf[8192];
        long n = slurp(w[i].path, buf, sizeof(buf));
        _unlink(w[i].path);

        EXPECT_GT(n, 16) << "suspiciously small file: " << w[i].path;
    }
}

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
