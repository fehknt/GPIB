#ifndef GFXFILE_H
#define GFXFILE_H

#include <stdio.h>
#include "typedefs.h"
#include "w32sal.h"
#include "winvfx.h"

// ------------------------------------------------------------------------
// General graphics file-related routines from various sources
//
// john@miles.io
// ------------------------------------------------------------------------

VFX_RGB * TGA_parse        (void *TGA_image, S32 *x_res, S32 *y_res);
bool      TGA_write_16bpp  (PANE *src, C8 *filename);
bool      GIF_write_16bpp  (PANE *src, C8 *filename);
bool      BMP_write_16bpp  (PANE *src, C8 *filename);
bool      PCX_write_16bpp  (PANE *src, C8 *filename);
bool      PNG_write_16bpp  (PANE *src, C8 *filename);   // encodes via GDI+, loaded at run time

#ifdef WINVFX_H
   #define WINVFX
#endif

#ifdef WINVFX
   #define PANE_DIMS(p,w,h) { w = (p)->x1-(p)->x0+1; h = (p)->y1-(p)->y0+1; }
#else
   #define PANE_DIMS(p,w,h) { w = (p)->width; h = (p)->height; }
#endif

#define WRITE_COMPRESSED_FILE 1     // 0=uncompressed, 1=RLE compressed

#pragma pack(1)  // Do NOT allow compiler to reorder structs!

struct TGA_HDR
{
   U8    len_image_ID;
   U8    color_map_present;
   U8    image_type;
   S16   color_map_origin;
   S16   color_map_len;
   U8    color_map_entry_size;
   S16   X_origin;
   S16   Y_origin;
   S16   pixel_width;
   S16   pixel_height;
   U8    bits_per_pixel;
   U8    image_descriptor_flags;
};

#define TGA_INTERLEAVE_4 0x80
#define TGA_INTERLEAVE_2 0x40
#define TGA_ORIGIN_TOP   0x20
#define TGA_ORIGIN_RIGHT 0x10
#define TGA_ATTRIB_MASK  0x0F

#define TGA_FORMAT_RGB_UNCOMPRESSED 2
#define TGA_FORMAT_RGB_RLE          10

//
// .GIF stuff from GraphApp
//

typedef unsigned char      byte;
typedef unsigned long      Char;

typedef struct GifColor      GifColor;

struct GifColor {
 byte  alpha;    /* transparency, 0=opaque, 255=transparent */
 byte  red;      /* intensity, 0=black, 255=bright red */
 byte  green;    /* intensity, 0=black, 255=bright green */
 byte  blue;     /* intensity, 0=black, 255=bright blue */
};

typedef struct {
    int      length;
    GifColor * colours;
  } GifPalette;

typedef struct {
    int          width, height;
    int          has_cmap, color_res, sorted, cmap_depth;
    int          bgcolour, aspect;
    GifPalette * cmap;
  } GifScreen;

typedef struct {
    int             byte_count;
    unsigned char * bytes;
  } GifData;

typedef struct {
    int        marker;
    int        data_count;
    GifData ** data;
  } GifExtension;

typedef struct {
    int              left, top, width, height;
    int              has_cmap, interlace, sorted, reserved, cmap_depth;
    GifPalette *     cmap;
    unsigned char ** data;
  } GifPicture;

typedef struct {
    int            intro;
    GifPicture *   pic;
    GifExtension * ext;
  } GifBlock;

typedef struct {
    char        header[8];
    GifScreen * screen;
    int         block_count;
    GifBlock ** blocks;
  } Gif;

/*
 *  Gif internal definitions:
 */

#define LZ_MAX_CODE     4095    /* Largest 12 bit code */
#define LZ_BITS         12

#define FLUSH_OUTPUT    4096    /* Impossible code = flush */
#define FIRST_CODE      4097    /* Impossible code = first */
#define NO_SUCH_CODE    4098    /* Impossible code = empty */

#define HT_SIZE         8192    /* 13 bit hash table size */
#define HT_KEY_MASK     0x1FFF  /* 13 bit key mask */

#define IMAGE_LOADING   0       /* file_state = processing */
#define IMAGE_SAVING    0       /* file_state = processing */
#define IMAGE_COMPLETE  1       /* finished reading or writing */

typedef struct {
    FILE *file;
    int depth,
        clear_code, eof_code,
        running_code, running_bits,
        max_code_plus_one,
   prev_code, current_code,
        stack_ptr,
        shift_state;
    unsigned long shift_data;
    unsigned long pixel_count;
    int           file_state, position, bufsize;
    unsigned char buf[256];
    unsigned long hash_table[HT_SIZE];
  } GifEncoder;

typedef struct {
    FILE *file;
    int depth,
        clear_code, eof_code,
        running_code, running_bits,
        max_code_plus_one,
        prev_code, current_code,
        stack_ptr,
        shift_state;
    unsigned long shift_data;
    unsigned long pixel_count;
    int           file_state, position, bufsize;
    unsigned char buf[256];
    unsigned char stack[LZ_MAX_CODE+1];
    unsigned char suffix[LZ_MAX_CODE+1];
    unsigned int  prefix[LZ_MAX_CODE+1];
  } GifDecoder;


void * gif_alloc(long bytes);

int    read_gif_int(FILE *file);
void   write_gif_int(FILE *file, int output);

GifData * new_gif_data(int size);
GifData * read_gif_data(FILE *file);
void   del_gif_data(GifData *data);
void   write_gif_data(FILE *file, GifData *data);
void   print_gif_data(FILE *file, GifData *data);

GifPalette * new_gif_palette(void);
void   del_gif_palette(GifPalette *cmap);
void   read_gif_palette(FILE *file, GifPalette *cmap);
void   write_gif_palette(FILE *file, GifPalette *cmap);
void   print_gif_palette(FILE *file, GifPalette *cmap);

GifScreen * new_gif_screen(void);
void   del_gif_screen(GifScreen *screen);
void   read_gif_screen(FILE *file, GifScreen *screen);
void   write_gif_screen(FILE *file, GifScreen *screen);
void   print_gif_screen(FILE *file, GifScreen *screen);

GifExtension *new_gif_extension(void);
void   del_gif_extension(GifExtension *ext);
void   read_gif_extension(FILE *file, GifExtension *ext);
void   write_gif_extension(FILE *file, GifExtension *ext);
void   print_gif_extension(FILE *file, GifExtension *ext);

GifDecoder * new_gif_decoder(void);
void   del_gif_decoder(GifDecoder *decoder);
void   init_gif_decoder(FILE *file, GifDecoder *decoder);

int   read_gif_code(FILE *file, GifDecoder *decoder);
void   read_gif_line(FILE *file, GifDecoder *decoder, unsigned char *line, int length);

GifEncoder * new_gif_encoder(void);
void   del_gif_encoder(GifEncoder *encoder);
void   write_gif_code(FILE *file, GifEncoder *encoder, int code);
void   init_gif_encoder(FILE *file, GifEncoder *encoder, int depth);
void   write_gif_line(FILE *file, GifEncoder *encoder, unsigned char *line, int length);
void   flush_gif_encoder(FILE *file, GifEncoder *encoder);

GifPicture * new_gif_picture(void);
void   del_gif_picture(GifPicture *pic);
void   read_gif_picture(FILE *file, GifPicture *pic);
void   write_gif_picture(FILE *file, GifPicture *pic);
void   print_gif_picture(FILE *file, GifPicture *pic);

GifBlock *new_gif_block(void);
void   del_gif_block(GifBlock *block);
void   read_gif_block(FILE *file, GifBlock *block);
void   write_gif_block(FILE *file, GifBlock *block);
void   print_gif_block(FILE *file, GifBlock *block);

Gif *   new_gif(void);
void   del_gif(Gif *gif);
void   read_gif(FILE *file, Gif *gif);
void   read_one_gif_picture(FILE *file, Gif *gif);
void   write_gif(FILE *file, Gif *gif);
void   print_gif(FILE *file, Gif *gif);

Gif *   read_gif_file(char *filename);
int     write_gif_file(char *filename, Gif *gif);

#pragma pack()

#ifndef RGBUTILS_H
#define RGBUTILS_H

class RGB_BOX
{
public:
    S32 r0;    // min value, exclusive
    S32 r1;    // max value, inclusive

    S32 g0;  
    S32 g1;

    S32 b0;  
    S32 b1;

    S32 vol;
};

class CQT
{
public:
   S32 table[33][33][33];
};

class CMAP
{
   S16       *best;
   S16       *second_best;
   VFX_RGB   *palette;
   U32        colors;

public:
   CMAP(VFX_RGB *palette, U32 colors);
  ~CMAP();

   U8 nearest_neighbor (VFX_RGB *triplet, S32 dither);
};

class CQ
{
   CQT wt;
   CQT mr;
   CQT mg;
   CQT mb;

   DOUBLE *m2;

   DOUBLE Var   (RGB_BOX *cube);
   void   M3d   (S32     *vwt,  S32 *vmr, S32 *vmg, S32 *vmb);
   S32    Vol   (RGB_BOX *cube,                  CQT *mmt);
   S32    Bottom(RGB_BOX *cube, U8 dir,          CQT *mmt);
   S32    Top   (RGB_BOX *cube, U8 dir, S32 pos, CQT *mmt);

   DOUBLE Maximize(RGB_BOX *cube,
                   U8       dir, 
                   S32      first, 
                   S32      last, 
                   S32     *cut,
                   S32      whole_r, 
                   S32      whole_g, 
                   S32      whole_b, 
                   S32      whole_w);

   S32 Cut(RGB_BOX *set1, RGB_BOX *set2);

public:
   CQ();
  ~CQ();

   void  reset     (void);
   void  add_color (VFX_RGB *triplet);
   U32   quantize  (VFX_RGB *out, U32 colors);
};

#define RGBU_RED   2
#define RGBU_GREEN 1   
#define RGBU_BLUE  0

#endif // RGBUTILS_H

#pragma pack(1)
typedef struct
   {
   unsigned char manufacturer;   // 10
   unsigned char version;        // 5
   unsigned char encoding;       // 1
   unsigned char bitsperpixel;   // 8
   short         x0;
   short         y0;
   short         x1;
   short         y1;             // size inclusive
   short         wide;
   short         tall;
   unsigned char colormap[48];
   unsigned char reserved;       // 0
   unsigned char numcolorplanes; // 1
   short         bytesperline;   // always even (but see note below...)
   short         paletteinfo;    // 1
   short         screenWide;
   short         screenTall;
   unsigned char filler[54];     // 0
}
PCXHEADER;
#pragma pack()

unsigned char *PCX_load (char *filename, int *wide, int *tall);
unsigned char *PCX_load_palette (char *filename);

#endif // GFXFILE_H
