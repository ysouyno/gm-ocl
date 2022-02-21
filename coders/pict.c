/*
% Copyright (C) 2003-2020 GraphicsMagick Group
% Copyright (C) 2002 ImageMagick Studio
% Copyright 1991-1999 E. I. du Pont de Nemours and Company
%
% This program is covered by multiple licenses, which are described in
% Copyright.txt. You should have received a copy of Copyright.txt with this
% package; otherwise see http://www.graphicsmagick.org/www/Copyright.html.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        PPPP   IIIII   CCCC  TTTTT                           %
%                        P   P    I    C        T                             %
%                        PPPP     I    C        T                             %
%                        P        I    C        T                             %
%                        P      IIIII   CCCC    T                             %
%                                                                             %
%                                                                             %
%              Read/Write Apple Macintosh QuickDraw/PICT Format.              %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/blob.h"
#include "magick/colormap.h"
#include "magick/composite.h"
#include "magick/constitute.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/monitor.h"
#include "magick/pixel_cache.h"
#include "magick/profile.h"
#include "magick/tempfile.h"
#include "magick/transform.h"
#include "magick/utility.h"

/*
  GraphicsMagick Macintosh PICT Methods.
*/
#define ReadPixmap(pixmap)                                             \
  {                                                                    \
  pixmap.version=ReadBlobMSBShort(image);                              \
  pixmap.pack_type=ReadBlobMSBShort(image);                            \
  pixmap.pack_size=ReadBlobMSBLong(image);                             \
  pixmap.horizontal_resolution=ReadBlobMSBLong(image);                 \
  pixmap.vertical_resolution=ReadBlobMSBLong(image);                   \
  pixmap.pixel_type=ReadBlobMSBShort(image);                           \
  pixmap.bits_per_pixel=ReadBlobMSBShort(image);                       \
  pixmap.component_count=ReadBlobMSBShort(image);                      \
  pixmap.component_size=ReadBlobMSBShort(image);                       \
  pixmap.plane_bytes=ReadBlobMSBLong(image);                           \
  pixmap.table=ReadBlobMSBLong(image);                                 \
  pixmap.reserved=ReadBlobMSBLong(image);                              \
  }

#define ValidatePixmap(pixmap) \
  (!(EOFBlob(image) ||                                                 \
     pixmap.bits_per_pixel <= 0 || pixmap.bits_per_pixel > 32 ||       \
     pixmap.component_count <= 0 || pixmap.component_count > 4 ||      \
     pixmap.component_size <= 0))

/*
  Read a PICT rectangle
*/
#define ReadRectangle(rectangle)                                       \
  {                                                                    \
  rectangle.top=ReadBlobMSBShort(image);                               \
  rectangle.left=ReadBlobMSBShort(image);                              \
  rectangle.bottom=ReadBlobMSBShort(image);                            \
  rectangle.right=ReadBlobMSBShort(image);                             \
  }

/*
  Return true if PICT rectangle is valid.
*/
#define ValidateRectangle(rectangle) \
  ((!EOFBlob(image) && \
    !((rectangle.bottom | rectangle.top | rectangle.right | rectangle.left ) & 0x8000) && \
    !(rectangle.bottom < rectangle.top) && \
    !(rectangle.right < rectangle.left)))

/*
  Issue a trace message with rectangle dimensions.
*/
#define TraceRectangle(image,rectangle)                         \
  do                                                            \
    {                                                           \
      if (image->logging)                                       \
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),     \
                              "%sRectangle: top %+d, bottom %+d, "        \
                              "left %+d, right %+d",            \
                              EOFBlob(image) ? "EOF! " : "",    \
                              (int) frame.top,                  \
                              (int) frame.bottom,               \
                              (int) frame.left,                 \
                              (int) frame.right);               \
    } while(0)

typedef struct _PICTPixmap
{
  magick_uint16_t
    version,
    pack_type;

  magick_uint32_t
    pack_size,
    horizontal_resolution,
    vertical_resolution;

  magick_uint16_t
    pixel_type,
    bits_per_pixel,
    component_count,
    component_size;

  magick_uint32_t
    plane_bytes,
    table,
    reserved;
} PICTPixmap;

typedef struct _PICTRectangle
{
  magick_uint16_t
    top,
    left,
    bottom,
    right;
} PICTRectangle;


/* Code Lengths */
static const magick_int8_t code_lengths[]=
  {
    /* 0x00 */ 0,
    /* 0x01 */ 0,
    /* 0x02 */ 8,
    /* 0x03 */ 2,
    /* 0x04 */ 1,
    /* 0x05 */ 2,
    /* 0x06 */ 4,
    /* 0x07 */ 4,
    /* 0x08 */ 2,
    /* 0x09 */ 8,
    /* 0x0A */ 8,
    /* 0x0B */ 4,
    /* 0x0C */ 4,
    /* 0x0D */ 2,
    /* 0x0E */ 4,
    /* 0x0F */ 4,
    /* 0x10 */ 8,
    /* 0x11 */ 1,
    /* 0x12 */ 0,
    /* 0x13 */ 0,
    /* 0x14 */ 0,
    /* 0x15 */ 2,
    /* 0x16 */ 2,
    /* 0x17 */ 0,
    /* 0x18 */ 0,
    /* 0x19 */ 0,
    /* 0x1A */ 6,
    /* 0x1B */ 6,
    /* 0x1C */ 0,
    /* 0x1D */ 6,
    /* 0x1E */ 0,
    /* 0x1F */ 6,
    /* 0x20 */ 8,
    /* 0x21 */ 4,
    /* 0x22 */ 6,
    /* 0x23 */ 2,
    /* 0x24 */ -1,
    /* 0x25 */ -1,
    /* 0x26 */ -1,
    /* 0x27 */ -1,
    /* 0x28 */ 0,
    /* 0x29 */ 0,
    /* 0x2A */ 0,
    /* 0x2B */ 0,
    /* 0x2C */ -1,
    /* 0x2D */ -1,
    /* 0x2E */ -1,
    /* 0x2F */ -1,
    /* 0x30 */ 8,
    /* 0x31 */ 8,
    /* 0x32 */ 8,
    /* 0x33 */ 8,
    /* 0x34 */ 8,
    /* 0x35 */ 8,
    /* 0x36 */ 8,
    /* 0x37 */ 8,
    /* 0x38 */ 0,
    /* 0x39 */ 0,
    /* 0x3A */ 0,
    /* 0x3B */ 0,
    /* 0x3C */ 0,
    /* 0x3D */ 0,
    /* 0x3E */ 0,
    /* 0x3F */ 0,
    /* 0x40 */ 8,
    /* 0x41 */ 8,
    /* 0x42 */ 8,
    /* 0x43 */ 8,
    /* 0x44 */ 8,
    /* 0x45 */ 8,
    /* 0x46 */ 8,
    /* 0x47 */ 8,
    /* 0x48 */ 0,
    /* 0x49 */ 0,
    /* 0x4A */ 0,
    /* 0x4B */ 0,
    /* 0x4C */ 0,
    /* 0x4D */ 0,
    /* 0x4E */ 0,
    /* 0x4F */ 0,
    /* 0x50 */ 8,
    /* 0x51 */ 8,
    /* 0x52 */ 8,
    /* 0x53 */ 8,
    /* 0x54 */ 8,
    /* 0x55 */ 8,
    /* 0x56 */ 8,
    /* 0x57 */ 8,
    /* 0x58 */ 0,
    /* 0x59 */ 0,
    /* 0x5A */ 0,
    /* 0x5B */ 0,
    /* 0x5C */ 0,
    /* 0x5D */ 0,
    /* 0x5E */ 0,
    /* 0x5F */ 0,
    /* 0x60 */ 12,
    /* 0x61 */ 12,
    /* 0x62 */ 12,
    /* 0x63 */ 12,
    /* 0x64 */ 12,
    /* 0x65 */ 12,
    /* 0x66 */ 12,
    /* 0x67 */ 12,
    /* 0x68 */ 4,
    /* 0x69 */ 4,
    /* 0x6A */ 4,
    /* 0x6B */ 4,
    /* 0x6C */ 4,
    /* 0x6D */ 4,
    /* 0x6E */ 4,
    /* 0x6F */ 4,
    /* 0x70 */ 0,
    /* 0x71 */ 0,
    /* 0x72 */ 0,
    /* 0x73 */ 0,
    /* 0x74 */ 0,
    /* 0x75 */ 0,
    /* 0x76 */ 0,
    /* 0x77 */ 0,
    /* 0x78 */ 0,
    /* 0x79 */ 0,
    /* 0x7A */ 0,
    /* 0x7B */ 0,
    /* 0x7C */ 0,
    /* 0x7D */ 0,
    /* 0x7E */ 0,
    /* 0x7F */ 0,
    /* 0x80 */ 0,
    /* 0x81 */ 0,
    /* 0x82 */ 0,
    /* 0x83 */ 0,
    /* 0x84 */ 0,
    /* 0x85 */ 0,
    /* 0x86 */ 0,
    /* 0x87 */ 0,
    /* 0x88 */ 0,
    /* 0x89 */ 0,
    /* 0x8A */ 0,
    /* 0x8B */ 0,
    /* 0x8C */ 0,
    /* 0x8D */ 0,
    /* 0x8E */ 0,
    /* 0x8F */ 0,
    /* 0x90 */ 0,
    /* 0x91 */ 0,
    /* 0x92 */ -1,
    /* 0x93 */ -1,
    /* 0x94 */ -1,
    /* 0x95 */ -1,
    /* 0x96 */ -1,
    /* 0x97 */ -1,
    /* 0x98 */ 0,
    /* 0x99 */ 0,
    /* 0x9A */ 0,
    /* 0x9B */ 0,
    /* 0x9C */ -1,
    /* 0x9D */ -1,
    /* 0x9E */ -1,
    /* 0x9F */ -1,
    /* 0xA0 */ 2,
    /* 0xA1 */ 0
  };

/* Code names */
static const char code_names[] =
  /* 0x00 */ "NOP\0"
  /* 0x01 */ "Clip\0"
  /* 0x02 */ "BkPat\0"
  /* 0x03 */ "TxFont\0"
  /* 0x04 */ "TxFace\0"
  /* 0x05 */ "TxMode\0"
  /* 0x06 */ "SpExtra\0"
  /* 0x07 */ "PnSize\0"
  /* 0x08 */ "PnMode\0"
  /* 0x09 */ "PnPat\0"
  /* 0x0A */ "FillPat\0"
  /* 0x0B */ "OvSize\0"
  /* 0x0C */ "Origin\0"
  /* 0x0D */ "TxSize\0"
  /* 0x0E */ "FgColor\0"
  /* 0x0F */ "BkColor\0"
  /* 0x10 */ "TxRatio\0"
  /* 0x11 */ "Version\0"
  /* 0x12 */ "BkPixPat\0"
  /* 0x13 */ "PnPixPat\0"
  /* 0x14 */ "FillPixPat\0"
  /* 0x15 */ "PnLocHFrac\0"
  /* 0x16 */ "ChExtra\0"
  /* 0x17 */ "reserved\0"
  /* 0x18 */ "reserved\0"
  /* 0x19 */ "reserved\0"
  /* 0x1A */ "RGBFgCol\0"
  /* 0x1B */ "RGBBkCol\0"
  /* 0x1C */ "HiliteMode\0"
  /* 0x1D */ "HiliteColor\0"
  /* 0x1E */ "DefHilite\0"
  /* 0x1F */ "OpColor\0"
  /* 0x20 */ "Line\0"
  /* 0x21 */ "LineFrom\0"
  /* 0x22 */ "ShortLine\0"
  /* 0x23 */ "ShortLineFrom\0"
  /* 0x24 */ "reserved\0"
  /* 0x25 */ "reserved\0"
  /* 0x26 */ "reserved\0"
  /* 0x27 */ "reserved\0"
  /* 0x28 */ "LongText\0"
  /* 0x29 */ "DHText\0"
  /* 0x2A */ "DVText\0"
  /* 0x2B */ "DHDVText\0"
  /* 0x2C */ "reserved\0"
  /* 0x2D */ "reserved\0"
  /* 0x2E */ "reserved\0"
  /* 0x2F */ "reserved\0"
  /* 0x30 */ "frameRect\0"
  /* 0x31 */ "paintRect\0"
  /* 0x32 */ "eraseRect\0"
  /* 0x33 */ "invertRect\0"
  /* 0x34 */ "fillRect\0"
  /* 0x35 */ "reserved\0"
  /* 0x36 */ "reserved\0"
  /* 0x37 */ "reserved\0"
  /* 0x38 */ "frameSameRect\0"
  /* 0x39 */ "paintSameRect\0"
  /* 0x3A */ "eraseSameRect\0"
  /* 0x3B */ "invertSameRect\0"
  /* 0x3C */ "fillSameRect\0"
  /* 0x3D */ "reserved\0"
  /* 0x3E */ "reserved\0"
  /* 0x3F */ "reserved\0"
  /* 0x40 */ "frameRRect\0"
  /* 0x41 */ "paintRRect\0"
  /* 0x42 */ "eraseRRect\0"
  /* 0x43 */ "invertRRect\0"
  /* 0x44 */ "fillRRrect\0"
  /* 0x45 */ "reserved\0"
  /* 0x46 */ "reserved\0"
  /* 0x47 */ "reserved\0"
  /* 0x48 */ "frameSameRRect\0"
  /* 0x49 */ "paintSameRRect\0"
  /* 0x4A */ "eraseSameRRect\0"
  /* 0x4B */ "invertSameRRect\0"
  /* 0x4C */ "fillSameRRect\0"
  /* 0x4D */ "reserved\0"
  /* 0x4E */ "reserved\0"
  /* 0x4F */ "reserved\0"
  /* 0x50 */ "frameOval\0"
  /* 0x51 */ "paintOval\0"
  /* 0x52 */ "eraseOval\0"
  /* 0x53 */ "invertOval\0"
  /* 0x54 */ "fillOval\0"
  /* 0x55 */ "reserved\0"
  /* 0x56 */ "reserved\0"
  /* 0x57 */ "reserved\0"
  /* 0x58 */ "frameSameOval\0"
  /* 0x59 */ "paintSameOval\0"
  /* 0x5A */ "eraseSameOval\0"
  /* 0x5B */ "invertSameOval\0"
  /* 0x5C */ "fillSameOval\0"
  /* 0x5D */ "reserved\0"
  /* 0x5E */ "reserved\0"
  /* 0x5F */ "reserved\0"
  /* 0x60 */ "frameArc\0"
  /* 0x61 */ "paintArc\0"
  /* 0x62 */ "eraseArc\0"
  /* 0x63 */ "invertArc\0"
  /* 0x64 */ "fillArc\0"
  /* 0x65 */ "reserved\0"
  /* 0x66 */ "reserved\0"
  /* 0x67 */ "reserved\0"
  /* 0x68 */ "frameSameArc\0"
  /* 0x69 */ "paintSameArc\0"
  /* 0x6A */ "eraseSameArc\0"
  /* 0x6B */ "invertSameArc\0"
  /* 0x6C */ "fillSameArc\0"
  /* 0x6D */ "reserved\0"
  /* 0x6E */ "reserved\0"
  /* 0x6F */ "reserved\0"
  /* 0x70 */ "framePoly\0"
  /* 0x71 */ "paintPoly\0"
  /* 0x72 */ "erasePoly\0"
  /* 0x73 */ "invertPoly\0"
  /* 0x74 */ "fillPoly\0"
  /* 0x75 */ "reserved\0"
  /* 0x76 */ "reserved\0"
  /* 0x77 */ "reserved\0"
  /* 0x78 */ "frameSamePoly\0"
  /* 0x79 */ "paintSamePoly\0"
  /* 0x7A */ "eraseSamePoly\0"
  /* 0x7B */ "invertSamePoly\0"
  /* 0x7C */ "fillSamePoly\0"
  /* 0x7D */ "reserved\0"
  /* 0x7E */ "reserved\0"
  /* 0x7F */ "reserved\0"
  /* 0x80 */ "frameRgn\0"
  /* 0x81 */ "paintRgn\0"
  /* 0x82 */ "eraseRgn\0"
  /* 0x83 */ "invertRgn\0"
  /* 0x84 */ "fillRgn\0"
  /* 0x85 */ "reserved\0"
  /* 0x86 */ "reserved\0"
  /* 0x87 */ "reserved\0"
  /* 0x88 */ "frameSameRgn\0"
  /* 0x89 */ "paintSameRgn\0"
  /* 0x8A */ "eraseSameRgn\0"
  /* 0x8B */ "invertSameRgn\0"
  /* 0x8C */ "fillSameRgn\0"
  /* 0x8D */ "reserved\0"
  /* 0x8E */ "reserved\0"
  /* 0x8F */ "reserved\0"
  /* 0x90 */ "BitsRect\0"
  /* 0x91 */ "BitsRgn\0"
  /* 0x92 */ "reserved\0"
  /* 0x93 */ "reserved\0"
  /* 0x94 */ "reserved\0"
  /* 0x95 */ "reserved\0"
  /* 0x96 */ "reserved\0"
  /* 0x97 */ "reserved\0"
  /* 0x98 */ "PackBitsRect\0"
  /* 0x99 */ "PackBitsRgn\0"
  /* 0x9A */ "DirectBitsRect\0"
  /* 0x9B */ "DirectBitsRgn\0"
  /* 0x9C */ "reserved\0"
  /* 0x9D */ "reserved\0"
  /* 0x9E */ "reserved\0"
  /* 0x9F */ "reserved\0"
  /* 0xA0 */ "ShortComment\0"
  /* 0xA1 */ "LongComment\0";

/* Code Descriptions */
static const char code_descriptions[] =
  /* 0x00 */ "nop\0"
  /* 0x01 */ "clip\0"
  /* 0x02 */ "background pattern\0"
  /* 0x03 */ "text font (word)\0"
  /* 0x04 */ "text face (byte)\0"
  /* 0x05 */ "text mode (word)\0"
  /* 0x06 */ "space extra (fixed point)\0"
  /* 0x07 */ "pen size (point)\0"
  /* 0x08 */ "pen mode (word)\0"
  /* 0x09 */ "pen pattern\0"
  /* 0x0A */ "fill pattern\0"
  /* 0x0B */ "oval size (point)\0"
  /* 0x0C */ "dh, dv (word)\0"
  /* 0x0D */ "text size (word)\0"
  /* 0x0E */ "foreground color (longword)\0"
  /* 0x0F */ "background color (longword)\0"
  /* 0x10 */ "numerator (point), denominator (point)\0"
  /* 0x11 */ "version (byte)\0"
  /* 0x12 */ "color background pattern\0"
  /* 0x13 */ "color pen pattern\0"
  /* 0x14 */ "color fill pattern\0"
  /* 0x15 */ "fractional pen position\0"
  /* 0x16 */ "extra for each character\0"
  /* 0x17 */ "reserved for Apple use\0"
  /* 0x18 */ "reserved for Apple use\0"
  /* 0x19 */ "reserved for Apple use\0"
  /* 0x1A */ "RGB foreColor\0"
  /* 0x1B */ "RGB backColor\0"
  /* 0x1C */ "hilite mode flag\0"
  /* 0x1D */ "RGB hilite color\0"
  /* 0x1E */ "Use default hilite color\0"
  /* 0x1F */ "RGB OpColor for arithmetic modes\0"
  /* 0x20 */ "pnLoc (point), newPt (point)\0"
  /* 0x21 */ "newPt (point)\0"
  /* 0x22 */ "pnLoc (point, dh, dv (-128 .. 127))\0"
  /* 0x23 */ "dh, dv (-128 .. 127)\0"
  /* 0x24 */ "reserved for Apple use\0"
  /* 0x25 */ "reserved for Apple use\0"
  /* 0x26 */ "reserved for Apple use\0"
  /* 0x27 */ "reserved for Apple use\0"
  /* 0x28 */ "txLoc (point), count (0..255), text\0"
  /* 0x29 */ "dh (0..255), count (0..255), text\0"
  /* 0x2A */ "dv (0..255), count (0..255), text\0"
  /* 0x2B */ "dh, dv (0..255), count (0..255), text\0"
  /* 0x2C */ "reserved for Apple use\0"
  /* 0x2D */ "reserved for Apple use\0"
  /* 0x2E */ "reserved for Apple use\0"
  /* 0x2F */ "reserved for Apple use\0"
  /* 0x30 */ "rect\0"
  /* 0x31 */ "rect\0"
  /* 0x32 */ "rect\0"
  /* 0x33 */ "rect\0"
  /* 0x34 */ "rect\0"
  /* 0x35 */ "reserved for Apple use\0"
  /* 0x36 */ "reserved for Apple use\0"
  /* 0x37 */ "reserved for Apple use\0"
  /* 0x38 */ "rect\0"
  /* 0x39 */ "rect\0"
  /* 0x3A */ "rect\0"
  /* 0x3B */ "rect\0"
  /* 0x3C */ "rect\0"
  /* 0x3D */ "reserved for Apple use\0"
  /* 0x3E */ "reserved for Apple use\0"
  /* 0x3F */ "reserved for Apple use\0"
  /* 0x40 */ "rect\0"
  /* 0x41 */ "rect\0"
  /* 0x42 */ "rect\0"
  /* 0x43 */ "rect\0"
  /* 0x44 */ "rect\0"
  /* 0x45 */ "reserved for Apple use\0"
  /* 0x46 */ "reserved for Apple use\0"
  /* 0x47 */ "reserved for Apple use\0"
  /* 0x48 */ "rect\0"
  /* 0x49 */ "rect\0"
  /* 0x4A */ "rect\0"
  /* 0x4B */ "rect\0"
  /* 0x4C */ "rect\0"
  /* 0x4D */ "reserved for Apple use\0"
  /* 0x4E */ "reserved for Apple use\0"
  /* 0x4F */ "reserved for Apple use\0"
  /* 0x50 */ "rect\0"
  /* 0x51 */ "rect\0"
  /* 0x52 */ "rect\0"
  /* 0x53 */ "rect\0"
  /* 0x54 */ "rect\0"
  /* 0x55 */ "reserved for Apple use\0"
  /* 0x56 */ "reserved for Apple use\0"
  /* 0x57 */ "reserved for Apple use\0"
  /* 0x58 */ "rect\0"
  /* 0x59 */ "rect\0"
  /* 0x5A */ "rect\0"
  /* 0x5B */ "rect\0"
  /* 0x5C */ "rect\0"
  /* 0x5D */ "reserved for Apple use\0"
  /* 0x5E */ "reserved for Apple use\0"
  /* 0x5F */ "reserved for Apple use\0"
  /* 0x60 */ "rect, startAngle, arcAngle\0"
  /* 0x61 */ "rect, startAngle, arcAngle\0"
  /* 0x62 */ "rect, startAngle, arcAngle\0"
  /* 0x63 */ "rect, startAngle, arcAngle\0"
  /* 0x64 */ "rect, startAngle, arcAngle\0"
  /* 0x65 */ "reserved for Apple use\0"
  /* 0x66 */ "reserved for Apple use\0"
  /* 0x67 */ "reserved for Apple use\0"
  /* 0x68 */ "rect, startAngle, arcAngle\0"
  /* 0x69 */ "rect, startAngle, arcAngle\0"
  /* 0x6A */ "rect, startAngle, arcAngle\0"
  /* 0x6B */ "rect, startAngle, arcAngle\0"
  /* 0x6C */ "rect, startAngle, arcAngle\0"
  /* 0x6D */ "reserved for Apple use\0"
  /* 0x6E */ "reserved for Apple use\0"
  /* 0x6F */ "reserved for Apple use\0"
  /* 0x70 */ "poly\0"
  /* 0x71 */ "poly\0"
  /* 0x72 */ "poly\0"
  /* 0x73 */ "poly\0"
  /* 0x74 */ "poly\0"
  /* 0x75 */ "reserved for Apple use\0"
  /* 0x76 */ "reserved for Apple use\0"
  /* 0x77 */ "reserved for Apple use\0"
  /* 0x78 */ "poly (NYI)\0"
  /* 0x79 */ "poly (NYI)\0"
  /* 0x7A */ "poly (NYI)\0"
  /* 0x7B */ "poly (NYI)\0"
  /* 0x7C */ "poly (NYI)\0"
  /* 0x7D */ "reserved for Apple use\0"
  /* 0x7E */ "reserved for Apple use\0"
  /* 0x7F */ "reserved for Apple use\0"
  /* 0x80 */ "region\0"
  /* 0x81 */ "region\0"
  /* 0x82 */ "region\0"
  /* 0x83 */ "region\0"
  /* 0x84 */ "region\0"
  /* 0x85 */ "reserved for Apple use\0"
  /* 0x86 */ "reserved for Apple use\0"
  /* 0x87 */ "reserved for Apple use\0"
  /* 0x88 */ "region (NYI)\0"
  /* 0x89 */ "region (NYI)\0"
  /* 0x8A */ "region (NYI)\0"
  /* 0x8B */ "region (NYI)\0"
  /* 0x8C */ "region (NYI)\0"
  /* 0x8D */ "reserved for Apple use\0"
  /* 0x8E */ "reserved for Apple use\0"
  /* 0x8F */ "reserved for Apple use\0"
  /* 0x90 */ "copybits, rect clipped\0"
  /* 0x91 */ "copybits, rgn clipped\0"
  /* 0x92 */ "reserved for Apple use\0"
  /* 0x93 */ "reserved for Apple use\0"
  /* 0x94 */ "reserved for Apple use\0"
  /* 0x95 */ "reserved for Apple use\0"
  /* 0x96 */ "reserved for Apple use\0"
  /* 0x97 */ "reserved for Apple use\0"
  /* 0x98 */ "packed copybits, rect clipped\0"
  /* 0x99 */ "packed copybits, rgn clipped\0"
  /* 0x9A */ "PixMap, srcRect, dstRect, mode, PixData\0"
  /* 0x9B */ "PixMap, srcRect, dstRect, mode, maskRgn, PixData\0"
  /* 0x9C */ "reserved for Apple use\0"
  /* 0x9D */ "reserved for Apple use\0"
  /* 0x9E */ "reserved for Apple use\0"
  /* 0x9F */ "reserved for Apple use\0"
  /* 0xA0 */ "kind (word)\0"
  /* 0xA1 */ "kind (word), size (word), data\0";

/*
  Forward declarations.
*/
static unsigned int
  WritePICTImage(const ImageInfo *,Image *);

static const char *lookup_string(const char *table, const size_t table_size, const unsigned int index)
{
    size_t count;
    const char *p = table;
    for (count = 0;
         (count < index) && (p < table+table_size-1);
         p++)
    {
        if (*p == '\0')
          count++;
    }
    return p;
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e c o d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method DecodeImage decompresses an image via Macintosh pack bits
%  decoding for Macintosh PICT images.
%
%  The format of the DecodeImage method is:
%
%      unsigned char* DecodeImage(const ImageInfo *image_info,Image *blob,
%        Image *image,unsigned long bytes_per_line,const int bits_per_pixel)
%
%  A description of each parameter follows:
%
%    o status:  Method DecodeImage returns True if all the pixels are
%      uncompressed without error, otherwise False.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o blob,image: The address of a structure of type Image.
%
%    o bytes_per_line: This integer identifies the number of bytes in a
%      scanline.
%
%    o bits_per_pixel: The number of bits in a pixel.
%
%
*/

static const unsigned char *ExpandBuffer(unsigned char *expand_buffer,
                                         const unsigned char * restrict pixels,
                                         unsigned long * restrict bytes_per_line,
                                         const unsigned int bits_per_pixel)
{
  register unsigned long
    i;

  register const unsigned char
    *p;

  register unsigned char
    *q;

  p=pixels;
  q=expand_buffer;
  switch (bits_per_pixel)
  {
    case 8:
    case 16:
    case 32:
      return(pixels);
    case 4:
    {
      for (i=0; i < *bytes_per_line; i++, p++)
      {
        *q++=(*p >> 4U) & 0xffU;
        *q++=(*p & 15U);
      }
      *bytes_per_line*=2;
      break;
    }
    case 2:
    {
      for (i=0; i < *bytes_per_line; i++, p++)
      {
        *q++=(*p >> 6U) & 0x03U;
        *q++=(*p >> 4U) & 0x03U;
        *q++=(*p >> 2U) & 0x03U;
        *q++=(*p & 3U);
      }
      *bytes_per_line*=4;
      break;
    }
    case 1:
    {
      for (i=0; i < *bytes_per_line; i++, p++)
      {
        *q++=(*p >> 7) & 0x01;
        *q++=(*p >> 6) & 0x01;
        *q++=(*p >> 5) & 0x01;
        *q++=(*p >> 4) & 0x01;
        *q++=(*p >> 3) & 0x01;
        *q++=(*p >> 2) & 0x01;
        *q++=(*p >> 1) & 0x01;
        *q++=(*p & 0x01);
      }
      *bytes_per_line*=8;
      break;
    }
    default:
      break;
  }
  return(expand_buffer);
}

static unsigned char *DecodeImage(const ImageInfo *image_info,
                                  Image *blob,Image *image,
                                  unsigned long bytes_per_line,
                                  const unsigned int bits_per_pixel)
{
  unsigned long
    y;

  register const unsigned char
    *p;

  register unsigned char
    *q;

  size_t
    allocated_pixels,
    scanline_alloc,
    row_bytes;

  unsigned char
    expand_buffer[8*256],
    *pixels = NULL,
    *scanline = NULL;

  unsigned long
    number_pixels,
    width;

  magick_off_t
    file_size;

  unsigned int
    bytes_per_pixel,
    i,
    j,
    length,
    scanline_length;

  ARG_NOT_USED(image_info);

  if (image->logging)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                          "DecodeImage: %lux%lu, bytes_per_line=%lu,"
                          " bits_per_pixel=%u",
                          image->columns, image->rows, bytes_per_line,
                          bits_per_pixel);

  /*
    Determine pixel buffer size.
  */
  if (bits_per_pixel <= 8)
    bytes_per_line&=0x7fff;
  width=image->columns;
  bytes_per_pixel=1;
  if (bits_per_pixel == 16)
    {
      bytes_per_pixel=2;
      width*=2;
    }
  else
    if (bits_per_pixel == 32)
      width*=image->matte ? 4 : 3;
  if (bytes_per_line == 0)
    bytes_per_line=width;
  row_bytes=(size_t) (image->columns | 0x8000);
  if (image->storage_class == DirectClass)
    row_bytes=(size_t) ((4*image->columns) | 0x8000);
  if (image->logging)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                          "DecodeImage: Using %lu bytes per line, %"
                          MAGICK_SIZE_T_F "u bytes per row",
                          bytes_per_line,
                          (MAGICK_SIZE_T) row_bytes);
  /*
    Validate allocation requests based on remaining file data
  */
  if ((file_size = GetBlobSize(blob)) > 0)
    {
      magick_off_t
        remaining;

      remaining=file_size-TellBlob(blob);

      if (remaining <= 0)
        {
          ThrowException(&image->exception,CorruptImageError,InsufficientImageDataInFile,
                         image->filename);
          goto decode_error_exit;
        }
      else
        {
          double
            ratio;

          ratio = (((double) image->rows*bytes_per_line)/remaining);

          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                "Remaining: %" MAGICK_OFF_F "d, Ratio: %g",
                                remaining, ratio);

          if (ratio > (bytes_per_line < 8 ? 1.0 : 255.0))
            {
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                    "Unreasonable file size "
                                    "(ratio of pixels to remaining file size %g)",
                                    ratio);
              ThrowException(&image->exception,CorruptImageError,InsufficientImageDataInFile,
                             image->filename);
              goto decode_error_exit;
            }
        }
    }

  /*
    Allocate pixel and scanline buffer.
  */
  allocated_pixels=MagickArraySize(image->rows,row_bytes);
  pixels=MagickAllocateClearedMemory(unsigned char *,allocated_pixels);
  if (pixels == (unsigned char *) NULL)
    {
      ThrowException(&image->exception,ResourceLimitError,MemoryAllocationFailed,
                     image->filename);
      goto decode_error_exit;
    }
  /* Use a worst-case allocation policy (because we can afford to) */
  if (bytes_per_line < 8)
    scanline_alloc = bytes_per_line;
  else if (bytes_per_line <= 200)
    scanline_alloc = 256U+256U; /* Allocate extra for RLE over-run */
  else
    scanline_alloc = 65536U+256U; /* Allocate extra for RLE over-run */

  scanline=MagickAllocateClearedMemory(unsigned char *,scanline_alloc);
  if (scanline == (unsigned char *) NULL)
    {
      ThrowException(&image->exception,ResourceLimitError,MemoryAllocationFailed,
                     image->filename);
      goto decode_error_exit;
    }
  (void) memset(expand_buffer,0,sizeof(expand_buffer));
  if (bytes_per_line < 8)
    {
      /*
        Pixels are already uncompressed.
      */
      for (y=0; y < image->rows; y++)
        {
          q=pixels+(size_t)y*width;
          number_pixels=bytes_per_line;
          if (ReadBlob(blob,number_pixels,(char *) scanline) != number_pixels)
            {
              ThrowException(&image->exception,CorruptImageError,UnexpectedEndOfFile,
                             image->filename);
              goto decode_error_exit;
            }
          p=ExpandBuffer(expand_buffer,scanline,&number_pixels,bits_per_pixel);
          (void) memcpy(q,p,number_pixels);
        }
      MagickFreeMemory(scanline);
      return(pixels);
    }
  /*
    Uncompress RLE pixels into uncompressed pixel buffer.
  */
  for (y=0; y < image->rows; y++)
    {
      q=pixels+(size_t)y*width;
      if (bytes_per_line > 200)
        scanline_length=ReadBlobMSBShort(blob);
      else
        scanline_length=ReadBlobByte(blob);
      if (image->logging)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                              "scanline_length = %u, "
                              "scanline_alloc = %"MAGICK_SIZE_T_F"u",
                              scanline_length, (MAGICK_SIZE_T)scanline_alloc);
      if (scanline_length < 2)
        {
          ThrowException(&image->exception,CorruptImageError,UnableToUncompressImage,
                         image->filename);
          goto decode_error_exit;
        }
      if (ReadBlob(blob,scanline_length,(char *) scanline) != scanline_length)
        {
          ThrowException(&image->exception,CorruptImageError,UnexpectedEndOfFile,
                         "Scanline length too small!");
          goto decode_error_exit;
        }
      #if 0
      (void) memset(scanline+scanline_length,0,scanline_alloc-scanline_length); /* Zero remainder */
      #endif
      for (j=0; j < scanline_length; )
        if ((scanline[j] & 0x80) == 0)
          {
            length=(scanline[j] & 0xff)+1;
            number_pixels=length*bytes_per_pixel;
            p=ExpandBuffer(expand_buffer,scanline+j+1,&number_pixels,bits_per_pixel);
            if (!((pixels+allocated_pixels)-number_pixels > q))
              {
                ThrowException(&image->exception,CorruptImageError,UnableToUncompressImage,
                               "Decoded RLE pixels exceeds allocation!");
                goto decode_error_exit;
              }
            (void) memcpy(q,p,number_pixels);
            q+=number_pixels;
            j+=length*bytes_per_pixel+1;
          }
        else
          {
            length=((scanline[j]^0xff) & 0xff)+2;
            number_pixels=bytes_per_pixel;
            p=ExpandBuffer(expand_buffer,scanline+j+1,&number_pixels,bits_per_pixel);
            for (i=0; i < length; i++)
              {
                if (!((pixels+allocated_pixels)-number_pixels > q))
                  {
                    ThrowException(&image->exception,CorruptImageError,UnableToUncompressImage,
                                   "Decoded RLE pixels exceeds allocation!");
                    goto decode_error_exit;
                  }
                (void) memcpy(q,p,number_pixels);
                q+=number_pixels;
              }
            j+=bytes_per_pixel+1;
          }
    }
  MagickFreeMemory(scanline);
  return (pixels);

 decode_error_exit:

  MagickFreeMemory(scanline);
  MagickFreeMemory(pixels);
  return (unsigned char *) NULL;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E n c o d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method EncodeImage compresses an image via Macintosh pack bits encoding
%  for Macintosh PICT images.
%
%  The format of the EncodeImage method is:
%
%      size_t EncodeImage(Image *image,const unsigned char *scanline,
%        const unsigned long bytes_per_line,unsigned char *pixels)
%
%  A description of each parameter follows:
%
%    o status:  Method EncodeImage returns the number of encoded pixels.
%
%    o image: The address of a structure of type Image.
%
%    o scanline: A pointer to an array of characters to pack.
%
%    o bytes_per_line: The number of bytes in a scanline.
%
%    o pixels: A pointer to an array of characters where the packed
%      characters are stored.
%
%
*/
static size_t EncodeImage(Image *image,const unsigned char *scanline,
  const size_t bytes_per_line,unsigned char *pixels)
{
#define MaxCount  128U
#define MaxPackbitsRunlength  128

  long
    count,
    repeat_count,
    runlength;

  register const unsigned char
    *p;

  register long
    i;

  register unsigned char
    *q;

  size_t
    length;

  unsigned char
    index;

  /*
    Pack scanline.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(scanline != (unsigned char *) NULL);
  assert(pixels != (unsigned char *) NULL);
  count=0;
  runlength=0;
  p=scanline+(bytes_per_line-1);
  q=pixels;
  index=(*p);
  for (i=(long) bytes_per_line-1; i >= 0; i--)
  {
    if (index == *p)
      runlength++;
    else
      {
        if (runlength < 3)
          while (runlength > 0)
          {
            *q++=(unsigned char) index;
            runlength--;
            count++;
            if (count == MaxCount)
              {
                *q++=(unsigned char) (MaxCount-1);
                count-=MaxCount;
              }
          }
        else
          {
            if (count > 0)
              *q++=(unsigned char) (count-1);
            count=0;
            while (runlength > 0)
            {
              repeat_count=runlength;
              if (repeat_count > MaxPackbitsRunlength)
                repeat_count=MaxPackbitsRunlength;
              *q++=(unsigned char) index;
              *q++=(unsigned char) (257-repeat_count);
              runlength-=repeat_count;
            }
          }
        runlength=1;
      }
    index=(*p);
    p--;
  }
  if (runlength < 3)
    while (runlength > 0)
    {
      *q++=(unsigned char) index;
      runlength--;
      count++;
      if (count == MaxCount)
        {
          *q++=(unsigned char) (MaxCount-1);
          count-=MaxCount;
        }
    }
  else
    {
      if (count > 0)
        *q++=(unsigned char) (count-1);
      count=0;
      while (runlength > 0)
      {
        repeat_count=runlength;
        if (repeat_count > MaxPackbitsRunlength)
          repeat_count=MaxPackbitsRunlength;
        *q++=(unsigned char) index;
        *q++=(unsigned char) (257-repeat_count);
        runlength-=repeat_count;
      }
    }
  if (count > 0)
    *q++= (unsigned char) (count-1);
  /*
    Write the number of and the packed length.
  */
  length=(q-pixels);
  if (bytes_per_line > 200)
    {
      (void) WriteBlobMSBShort(image,(const magick_uint16_t) length);
      length+=2;
    }
  else
    {
      (void) WriteBlobByte(image,(const magick_uint8_t) length);
      length++;
    }
  while (q != pixels)
  {
    q--;
    (void) WriteBlobByte(image,*q);
  }
  return(length);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P I C T I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadPICTImage reads an Apple Macintosh QuickDraw/PICT image file
%  and returns it.  It allocates the memory necessary for the new Image
%  structure and returns a pointer to the new image.
%
%  The format of the ReadPICTImage method is:
%
%      Image *ReadPICTImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image:  Method ReadPICTImage returns a pointer to the image after
%      reading.  A null image is returned if there is a memory shortage or
%      if the image cannot be read.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
#define ThrowPICTReaderException(code_,reason_,image_) \
{ \
  if (clone_info) \
    DestroyImageInfo(clone_info); \
  if (tile_image) \
    DestroyImage(tile_image); \
  ThrowReaderException(code_,reason_,image_); \
}

static Image *ReadPICTImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  char
    geometry[MaxTextExtent];

  Image
    *image = (Image *) NULL,
    *tile_image = (Image *) NULL;

  ImageInfo
    *clone_info = (ImageInfo *) NULL;

  IndexPacket
    index;

  int
    c,
    version;

  unsigned int
    code,
    flags;

  long
    j,
    y;

  PICTRectangle
    frame;

  PICTPixmap
    pixmap;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  register long
    i;

  size_t
    length;

  magick_off_t
    file_size;

  unsigned int
    jpeg,
    status;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == False)
    ThrowPICTReaderException(FileOpenError,UnableToOpenFile,image);
  file_size=GetBlobSize(image);
  pixmap.bits_per_pixel=0;
  pixmap.component_count=0;
  /*
    Read PICT header.
  */
  pixmap.bits_per_pixel=0;
  pixmap.component_count=0;
  for (i=0; i < 512; i++)
    (void) ReadBlobByte(image);  /* skip header */
  (void) ReadBlobMSBShort(image);  /* skip picture size */
  ReadRectangle(frame);
  TraceRectangle(image,frame);
  if (!ValidateRectangle(frame))
    ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
  while ((c=ReadBlobByte(image)) == 0);
  if (c != 0x11)
    ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
  version=ReadBlobByte(image);
  if (version == 2)
    {
      c=ReadBlobByte(image);
      if (c != 0xff)
        ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
    }
  else
    if (version != 1)
      ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
  /*
    Create black canvas.
  */
  flags=0;
  image->columns=frame.right-frame.left;
  image->rows=frame.bottom-frame.top;

  if (IsEventLogging())
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                          "Dimensions: %lux%lu",image->columns,image->rows);

  if (CheckImagePixelLimits(image, exception) != MagickPass)
    ThrowPICTReaderException(ResourceLimitError,ImagePixelLimitExceeded,image);

  SetRedSample(&image->background_color,0);
  SetGreenSample(&image->background_color,0);
  SetBlueSample(&image->background_color,0);
  SetOpacitySample(&image->background_color,OpaqueOpacity);
  if (SetImageEx(image,OpaqueOpacity,exception) != MagickPass)
    {
      CloseBlob(image);
      DestroyImage(image);
      return (Image *) NULL;
    }

  /*
    Interpret PICT opcodes.
  */
  jpeg=False;
  for (code=0; EOFBlob(image) == False; )
  {
    if (image_info->ping && (image_info->subrange != 0))
      if (image->scene >= (image_info->subimage+image_info->subrange-1))
        break;
    if ((version == 1) || ((TellBlob(image) % 2) != 0))
      {
        if ((c=ReadBlobByte(image)) == EOF) /* returns int */
          break;
        code=(unsigned int) c;
      }
    if (version == 2)
      code=ReadBlobMSBShort(image); /* returns magick_uint16_t */
    if (code > 0xa1)
      {
        if (IsEventLogging())
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Code %04X:",code);
      }
    else
      {
        if (IsEventLogging())
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Code  %04X %.1024s: %.1024s",code,
                                lookup_string(code_names,sizeof(code_names),code),
                                lookup_string(code_descriptions,sizeof(code_descriptions),code));
        switch (code)
        {
          case 0x01:
          {
            /*
              Clipping rectangle.
            */
            length=ReadBlobMSBShort(image);
            if (length != 0x000a)
              {
                for (i=0; i < (long) (length-2); i++)
                  if (ReadBlobByte(image) == EOF)
                    break;
                break;
              }
            ReadRectangle(frame);
            TraceRectangle(image,frame);
            if (!ValidateRectangle(frame))
              ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
            if ((frame.left & 0x8000) || (frame.top & 0x8000))
              break;
            image->columns=frame.right-frame.left;
            image->rows=frame.bottom-frame.top;
            if (CheckImagePixelLimits(image, exception) != MagickPass)
              ThrowPICTReaderException(ResourceLimitError,ImagePixelLimitExceeded,image);
            (void) SetImageEx(image,OpaqueOpacity,exception);
            break;
          }
          case 0x12:
          case 0x13:
          case 0x14:
          {
            long
              pattern;

            unsigned long
              height,
              width;

            /*
              Skip pattern definition.
            */
            pattern=ReadBlobMSBShort(image);
            for (i=0; i < 8; i++)
              (void) ReadBlobByte(image);
            if (pattern == 2)
              {
                for (i=0; i < 5; i++)
                  (void) ReadBlobByte(image);
                break;
              }
            if (pattern != 1)
              ThrowPICTReaderException(CorruptImageError,UnknownPatternType,
                image);
            length=ReadBlobMSBShort(image);
            ReadRectangle(frame);
            TraceRectangle(image,frame);
            if (!ValidateRectangle(frame))
              ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
            ReadPixmap(pixmap);
            if (!ValidatePixmap(pixmap))
              ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
            (void) ReadBlobMSBLong(image);
            flags=ReadBlobMSBShort(image);
            length=ReadBlobMSBShort(image);
            for (i=0; i <= (long) length; i++)
              (void) ReadBlobMSBLong(image);
            width=frame.bottom-frame.top;
            height=frame.right-frame.left;
            image->depth=pixmap.bits_per_pixel <= 8 ? 8 : QuantumDepth;
            if (pixmap.bits_per_pixel < 8)
              image->depth=8;
            if (pixmap.bits_per_pixel <= 8)
              length&=0x7fff;
            if (pixmap.bits_per_pixel == 16)
              width<<=1;
            if (length == 0)
              length=width;
            if (length < 8)
              {
                for (i=0; i < (long) (length*height); i++)
                  if (ReadBlobByte(image) == EOF)
                    break;
              }
            else
              {
                for (j=0; j < (int) height; j++)
                  {
                    if (EOFBlob(image))
                      break;
                    if (length > 200)
                      {
                        for (j=0; j < ReadBlobMSBShort(image); j++)
                          if (ReadBlobByte(image) == EOF)
                            break;
                      }
                    else
                      {
                        for (j=0; j < ReadBlobByte(image); j++)
                          if (ReadBlobByte(image) == EOF)
                            break;
                      }
                  }
              }
            break;
          }
          case 0x1b:
          {
            /*
              Initialize image background color.
            */
            image->background_color.red=(Quantum)
              ScaleShortToQuantum(ReadBlobMSBShort(image));
            image->background_color.green=(Quantum)
              ScaleShortToQuantum(ReadBlobMSBShort(image));
            image->background_color.blue=(Quantum)
              ScaleShortToQuantum(ReadBlobMSBShort(image));
            break;
          }
          case 0x70:
          case 0x71:
          case 0x72:
          case 0x73:
          case 0x74:
          case 0x75:
          case 0x76:
          case 0x77:
          {
            /*
              Skip polygon or region.
            */
            length=ReadBlobMSBShort(image);
            for (i=0; i < (long) (length-2); i++)
              if (ReadBlobByte(image) == EOF)
                break;
            break;
          }
          case 0x90:
          case 0x91:
          case 0x98:
          case 0x99:
          case 0x9a:
          case 0x9b:
          {
            long
              bytes_per_line;

            PICTRectangle
              source,
              destination;

            register unsigned char
              *p;

            size_t
              j;

            unsigned char
              *pixels;

            /*
              Pixmap clipped by a rectangle.
            */
            bytes_per_line=0;
            if ((code != 0x9a) && (code != 0x9b))
              bytes_per_line=ReadBlobMSBShort(image);
            else
              {
                (void) ReadBlobMSBShort(image);
                (void) ReadBlobMSBShort(image);
                (void) ReadBlobMSBShort(image);
              }
            ReadRectangle(frame);
            TraceRectangle(image,frame);
            if (!ValidateRectangle(frame))
              ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
            /*
              Initialize tile image.
            */
            tile_image=CloneImage(image,frame.right-frame.left,
              frame.bottom-frame.top,True,exception);
            if (tile_image == (Image *) NULL)
              {
                DestroyImage(image);
                return((Image *) NULL);
              }
            DestroyBlob(tile_image);
            tile_image->blob=CloneBlobInfo((BlobInfo *) NULL);
            if ((code == 0x9a) || (code == 0x9b) || (bytes_per_line & 0x8000))
              {
                ReadPixmap(pixmap);
                if (!ValidatePixmap(pixmap))
                  ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
                tile_image->matte=pixmap.component_count == 4;
              }
            if ((code != 0x9a) && (code != 0x9b))
              {
                /*
                  Initialize colormap.
                */
                tile_image->colors=2;
                if (bytes_per_line & 0x8000)
                  {
                    (void) ReadBlobMSBLong(image);
                    flags=ReadBlobMSBShort(image);
                    tile_image->colors=ReadBlobMSBShort(image)+1;
                  }
                if (!AllocateImageColormap(tile_image,tile_image->colors))
                  ThrowPICTReaderException(ResourceLimitError,MemoryAllocationFailed,image)
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "Allocated tile image colormap with %u colors",tile_image->colors);
                if (bytes_per_line & 0x8000)
                  {
                    for (i=0; i < (long) tile_image->colors; i++)
                    {
                      j=ReadBlobMSBShort(image) % tile_image->colors;
                      if (flags & 0x8000)
                        j=i;
                      tile_image->colormap[j].red=(Quantum)
                        ScaleShortToQuantum(ReadBlobMSBShort(image));
                      tile_image->colormap[j].green=(Quantum)
                        ScaleShortToQuantum(ReadBlobMSBShort(image));
                      tile_image->colormap[j].blue=(Quantum)
                        ScaleShortToQuantum(ReadBlobMSBShort(image));
                      if (EOFBlob(image))
                        break;
                    }
                  }
                else
                  {
                    for (i=0; i < (long) tile_image->colors; i++)
                    {
                      tile_image->colormap[i].red=(Quantum) (MaxRGB-
                        tile_image->colormap[i].red);
                      tile_image->colormap[i].green=(Quantum) (MaxRGB-
                        tile_image->colormap[i].green);
                      tile_image->colormap[i].blue=(Quantum) (MaxRGB-
                        tile_image->colormap[i].blue);
                    }
                  }
              }
            if (EOFBlob(image))
              ThrowPICTReaderException(CorruptImageError,UnexpectedEndOfFile,image);
            ReadRectangle(source);
            TraceRectangle(image,source);
            if (!ValidateRectangle(source))
              ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
            ReadRectangle(destination);
            TraceRectangle(image,destination);
            if (!ValidateRectangle(destination))
              ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
            (void) ReadBlobMSBShort(image);
            if ((code == 0x91) || (code == 0x99) || (code == 0x9b))
              {
                /*
                  Skip region.
                */
                length=ReadBlobMSBShort(image);
                for (i=0; i <= (long) (length-2); i++)
                  if (ReadBlobByte(image) == EOF)
                    break;
              }
            if (CheckImagePixelLimits(tile_image, exception) != MagickPass)
              ThrowPICTReaderException(ResourceLimitError,ImagePixelLimitExceeded,image);
            if ((code != 0x9a) && (code != 0x9b) &&
                (bytes_per_line & 0x8000) == 0)
              pixels=DecodeImage(image_info,image,tile_image,bytes_per_line,1);
            else
              pixels=DecodeImage(image_info,image,tile_image,bytes_per_line,
                pixmap.bits_per_pixel);
            if (pixels == (unsigned char *) NULL)
              {
                CopyException(exception, &tile_image->exception);
                ThrowPICTReaderException(ResourceLimitError,MemoryAllocationFailed,image)
              }
            /*
              Convert PICT tile image to pixel packets.
            */
            p=pixels;
            for (y=0; y < (long) tile_image->rows; y++)
            {
              q=SetImagePixelsEx(tile_image,0,y,tile_image->columns,1,&image->exception);
              if (q == (PixelPacket *) NULL)
                break;
              indexes=AccessMutableIndexes(tile_image);
              for (x=0; x < (long) tile_image->columns; x++)
              {
                if (tile_image->storage_class == PseudoClass)
                  {
                    index=(IndexPacket) (*p);
                    VerifyColormapIndex(tile_image,index);
                    indexes[x]=index;
                    q->red=tile_image->colormap[index].red;
                    q->green=tile_image->colormap[index].green;
                    q->blue=tile_image->colormap[index].blue;
                  }
                else
                  {
                    if (pixmap.bits_per_pixel == 16)
                      {
                        i=(*p++);
                        j=(*p);
                        q->red=ScaleCharToQuantum((i & 0x7c) << 1);
                        q->green=ScaleCharToQuantum((((size_t)i & 0x03) << 6) |
                          (((size_t) j & 0xe0) >> 2));
                        q->blue=ScaleCharToQuantum((j & 0x1f) << 3);
                      }
                    else
                      if (!tile_image->matte)
                        {
                          q->red=ScaleCharToQuantum(*p);
                          q->green=
                            ScaleCharToQuantum(*(p+tile_image->columns));
                          q->blue=ScaleCharToQuantum(*(p+ (size_t)2*tile_image->columns));
                        }
                      else
                        {
                          q->opacity=(Quantum) (MaxRGB-ScaleCharToQuantum(*p));
                          q->red=ScaleCharToQuantum(*(p+tile_image->columns));
                          q->green=(Quantum)
                            ScaleCharToQuantum(*(p+ (size_t)2*tile_image->columns));
                          q->blue=
                           ScaleCharToQuantum(*(p+ (size_t)3*tile_image->columns));
                        }
                  }
                p++;
                q++;
              }
              if (!SyncImagePixelsEx(tile_image,&image->exception))
                break;
              if ((tile_image->storage_class == DirectClass) &&
                  (pixmap.bits_per_pixel != 16))
                p+=((size_t) pixmap.component_count-1)*tile_image->columns;
              if (destination.bottom == (long) image->rows)
                if (QuantumTick(y,tile_image->rows))
                  if (!MagickMonitorFormatted(y,tile_image->rows,&image->exception,
                                              LoadImageText,image->filename,
                                              image->columns,image->rows))
                    break;
            }
            MagickFreeMemory(pixels);
            if (tile_image->exception.severity > image->exception.severity)
              CopyException(&image->exception,&tile_image->exception);
            if ((tile_image->exception.severity < ErrorException) && (jpeg == False))
              if ((code == 0x9a) || (code == 0x9b) ||
                  (bytes_per_line & 0x8000))
                {
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                        "Composite tile: %lux%lu%+d%+d",
                                        tile_image->columns, tile_image->rows,
                                        destination.left, destination.top);
                  (void) CompositeImage(image,CopyCompositeOp,tile_image,
                                        destination.left,destination.top);
                }
            DestroyImage(tile_image);
            tile_image=(Image *) NULL;
            if (destination.bottom != (long) image->rows)
              if (!MagickMonitorFormatted(destination.bottom,image->rows,&image->exception,
                                          LoadImageText,image->filename,
                                          image->columns,image->rows))
                break;
            break;
          }
          case 0xa1:
          {
            unsigned char
              *info;

            unsigned int
              type;

            /*
              Comment.
            */
            type=ReadBlobMSBShort(image);
            length=ReadBlobMSBShort(image);
            if (length == 0)
              break;
            (void) ReadBlobMSBLong(image);
            length-=Min(4,length);
            if (length == 0)
              break;
            info=MagickAllocateMemory(unsigned char *,length);
            if (info == (unsigned char *) NULL)
              break;
            (void) ReadBlob(image,length,info);
            switch (type)
            {
              case 0xe0:
              {
                if (length == 0)
                  break;
                if (SetImageProfile(image,"ICM",info,length) == MagickFail)
                  ThrowPICTReaderException(ResourceLimitError,MemoryAllocationFailed,image);
                MagickFreeMemory(info);
                break;
              }
              case 0x1f2:
              {
                if (length == 0)
                  break;
                if (SetImageProfile(image,"IPTC",info,length) == MagickFail)
                  ThrowPICTReaderException(ResourceLimitError,MemoryAllocationFailed,image);
                MagickFreeMemory(info);
                break;
              }
              default:
                break;
            }
            MagickFreeMemory(info);
            break;
          }
          default:
          {
            /*
              Skip to next op code.
            */
            if (code_lengths[code] == -1)
              (void) ReadBlobMSBShort(image);
            else
              for (i=0; i < (long) code_lengths[code]; i++)
                if (ReadBlobByte(image) == EOF)
                  break;
          }
        }
      }
    if (code == 0xc00)
      {
        /*
          Skip header.
        */
        for (i=0; i < 24; i++)
          if (ReadBlobByte(image) == EOF)
            break;
        continue;
      }
    if (((code >= 0xb0) && (code <= 0xcf)) ||
        ((code >= 0x8000) && (code <= 0x80ff)))
      continue;
    if (code == 0x8200)
      {
        /*
          Embedded JPEG.
        */
        jpeg=True;
        length=ReadBlobMSBLong(image);
        if ((length > 154) && ((file_size <= 0) || ((size_t) (file_size-TellBlob(image)) > length)))
          {
            void
              *blob,
              *blob_alloc;

            const size_t
              blob_alloc_size = length-154;

            for (i=0; i < 6; i++)
              (void) ReadBlobMSBLong(image);
            ReadRectangle(frame);
            TraceRectangle(image,frame);
            if (!ValidateRectangle(frame))
              ThrowPICTReaderException(CorruptImageError,ImproperImageHeader,image);
            for (i=0; i < 122; i++)
              if (ReadBlobByte(image) == EOF)
                ThrowPICTReaderException(CorruptImageError,UnexpectedEndOfFile,image);
            if ((blob_alloc=MagickAllocateMemory(void *,blob_alloc_size)) == (void *) NULL)
              ThrowPICTReaderException(ResourceLimitError,MemoryAllocationFailed,image);
            blob=blob_alloc;
            clone_info=CloneImageInfo(image_info);
            clone_info->blob=(void *) NULL;
            clone_info->length=0;
            (void) strlcpy(clone_info->filename,"JPEG:",sizeof(clone_info->filename));
            if (ReadBlobZC(image,blob_alloc_size,&blob) != blob_alloc_size)
              {
                MagickFreeMemory(blob_alloc);
                ThrowPICTReaderException(CorruptImageError,UnexpectedEndOfFile,image);
              }
            if (blob != blob_alloc)
              {
                if (image->logging)
                  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                        "Tile Zero copy read.");
              }
            tile_image=BlobToImage(clone_info, blob, blob_alloc_size, &image->exception );
            DestroyImageInfo(clone_info);
            clone_info=(ImageInfo *) NULL;
            MagickFreeMemory(blob_alloc);
          }
        if (tile_image == (Image *) NULL)
          continue;
        if (IsEventLogging())
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                "Tile Dimensions: %lux%lu",
                                tile_image->columns,tile_image->rows);
        if (IsEventLogging())
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                "Tile Resolution: %gx%g %s",
                                tile_image->x_resolution,
                                tile_image->y_resolution,
                                tile_image->units == PixelsPerInchResolution ?
                                "pixels/inch" :
                                (tile_image->units == PixelsPerCentimeterResolution ?
                                 "pixels/centimeter" :
                                 "pixels"));
        FormatString(geometry,"%lux%lu",Max(image->columns,tile_image->columns),
          Max(image->rows,tile_image->rows));
        if (IsEventLogging())
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                "Tile Transform %lux%lu ==> %s",
                                tile_image->columns,tile_image->rows,
                                geometry);
        if (TransformImage(&tile_image,(char *) NULL,geometry) != MagickPass)
          {
            if (IsEventLogging())
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                    "Tile transform failed!");
          }
        if (IsEventLogging())
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                "Tile Composite of %lux%lu on canvas %lux%lu at +%u,+%u",
                                tile_image->columns,tile_image->rows,
                                image->columns,image->rows,frame.left,frame.right);
        if (CompositeImage(image,CopyCompositeOp,tile_image,frame.left,
                           frame.right) != MagickPass)
          {
            if (IsEventLogging())
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                    "Tile composite failed!");
          }
        image->compression=tile_image->compression;
        DestroyImage(tile_image);
        tile_image=(Image *) NULL;
        continue;
      }
    if ((code == 0xff) || (code == 0xffff))
      break;
    if (((code >= 0xd0) && (code <= 0xfe)) ||
        ((code >= 0x8100) && (code <= 0xffff)))
      {
        /*
          Skip reserved.
        */
        length=ReadBlobMSBShort(image);
        for (i=0; i < (long) length; i++)
          if (ReadBlobByte(image) == EOF)
            break;
        continue;
      }
    if ((code >= 0x100) && (code <= 0x7fff))
      {
        /*
          Skip reserved.
        */
        length=(code >> 7) & 0xff;
        for (i=0; i < (long) length; i++)
          if (ReadBlobByte(image) == EOF)
            break;
        continue;
      }
  }
  if (EOFBlob(image))
    ThrowException(exception,CorruptImageError,UnexpectedEndOfFile,
      image->filename);
  CloseBlob(image);
  StopTimer(&image->timer);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P I C T I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RegisterPICTImage adds attributes for the PICT image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPICTImage method is:
%
%      RegisterPICTImage(void)
%
*/
ModuleExport void RegisterPICTImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("PCT");
  entry->decoder=(DecoderHandler) ReadPICTImage;
  entry->encoder=(EncoderHandler) WritePICTImage;
  entry->adjoin=False;
  entry->description="Apple Macintosh QuickDraw/PICT";
  entry->seekable_stream=MagickTrue;
  entry->module="PICT";
  (void) RegisterMagickInfo(entry);

  entry=SetMagickInfo("PICT");
  entry->decoder=(DecoderHandler) ReadPICTImage;
  entry->encoder=(EncoderHandler) WritePICTImage;
  entry->adjoin=False;
  entry->description="Apple Macintosh QuickDraw/PICT";
  entry->seekable_stream=MagickTrue;
  entry->module="PICT";
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P I C T I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method UnregisterPICTImage removes format registrations made by the
%  PICT module from the list of supported formats.
%
%  The format of the UnregisterPICTImage method is:
%
%      UnregisterPICTImage(void)
%
*/
ModuleExport void UnregisterPICTImage(void)
{
  (void) UnregisterMagickInfo("PCT");
  (void) UnregisterMagickInfo("PICT");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P I C T I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WritePICTImage writes an image to a file in the Apple Macintosh
%  QuickDraw/PICT image format.
%
%  The format of the WritePICTImage method is:
%
%      unsigned int WritePICTImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o status: Method WritePICTImage return True if the image is written.
%      False is returned is there is a memory shortage or if the image file
%      fails to write.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o image:  A pointer to an Image structure.
%
%
*/
#define LiberatePICTAllocations()               \
  {                                             \
    MagickFreeMemory(buffer);                   \
    MagickFreeMemory(packed_scanline);          \
    MagickFreeMemory(scanline);                 \
  }
#define ThrowPICTWriterException(code_,reason_,image_)  \
  {                                                     \
    LiberatePICTAllocations();                          \
    ThrowWriterException(code_,reason_,image_);         \
  }
static unsigned int WritePICTImage(const ImageInfo *image_info,Image *image)
{
#define MaxCount  128U
#define PictCropRegionOp  0x01
#define PictEndOfPictureOp  0xff
#define PictJPEGOp  0x8200
#define PictInfoOp  0x0C00
#define PictInfoSize  512
#define PictPixmapOp  0x9A
#define PictPICTOp  0x98
#define PictVersion  0x11

  double
    x_resolution = 72.0,
    y_resolution = 72.0;

  long
    y;

  ExtendedSignedIntegralType
    offset;

  PICTPixmap
    pixmap;

  PICTRectangle
    bounds,
    crop_rectangle,
    destination_rectangle,
    frame_rectangle,
    size_rectangle,
    source_rectangle;

  const unsigned char
    *profile_info;

  size_t
    profile_length;

  register const PixelPacket
    *p;

  register const IndexPacket
    *indexes;

  register long
    i,
    x;

  size_t
    bytes_per_line,
    count,
    row_bytes;

  unsigned char
    *buffer = (unsigned char *) NULL,
    *packed_scanline = (unsigned char *) NULL,
    *scanline = (unsigned char *) NULL;

  unsigned int
    status;

  unsigned long
    storage_class;

  unsigned short
    base_address,
    transfer_mode;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if ((image->columns > 65535L) || (image->rows > 65535L))
    ThrowPICTWriterException(ImageError,WidthOrHeightExceedsLimit,image);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == False)
    ThrowPICTWriterException(FileOpenError,UnableToOpenFile,image);
  (void) TransformColorspace(image,RGBColorspace);
  /*
    Initialize image info.
  */
  size_rectangle.top=0;
  size_rectangle.left=0;
  size_rectangle.bottom=(short) image->rows;
  size_rectangle.right=(short) image->columns;
  frame_rectangle=size_rectangle;
  crop_rectangle=size_rectangle;
  source_rectangle=size_rectangle;
  destination_rectangle=size_rectangle;
  base_address=0xff;
  row_bytes=(size_t) image->columns;
  bounds.top=0;
  bounds.left=0;
  bounds.bottom=(short) image->rows;
  bounds.right=(short) image->columns;
  pixmap.version=0;
  pixmap.pack_type=0;
  pixmap.pack_size=0;
  pixmap.pixel_type=0;
  pixmap.bits_per_pixel=8;
  pixmap.component_count=1;
  pixmap.component_size=8;
  pixmap.plane_bytes=0;
  pixmap.table=0;
  pixmap.reserved=0;
  transfer_mode=0;
  if ((image->x_resolution > MagickEpsilon) &&
      (image->y_resolution > MagickEpsilon))
    {
      x_resolution=image->x_resolution;
      y_resolution=image->y_resolution;
      if (image->units == PixelsPerCentimeterResolution)
        {
          x_resolution *= 2.54;
          y_resolution *= 2.54;
        }
      x_resolution=ConstrainToRange(0.0,(double) 0xffff,x_resolution);
      y_resolution=ConstrainToRange(0.0,(double) 0xffff,y_resolution);
    }
  storage_class=image->storage_class;
  if (image->compression == JPEGCompression)
    storage_class=DirectClass;
  if (storage_class == DirectClass)
    {
      pixmap.component_count=image->matte ? 4 : 3;
      pixmap.pixel_type=16;
      pixmap.bits_per_pixel=32;
      pixmap.pack_type=0x04;
      transfer_mode=0x40;
      row_bytes=MagickArraySize(4,(size_t) image->columns);
      if (row_bytes == 0)
        ThrowPICTWriterException(ResourceLimitError,MemoryAllocationFailed,image);
    }
  /*
    Allocate memory.
  */
  bytes_per_line=(size_t) image->columns;
  if (storage_class == DirectClass)
    bytes_per_line = MagickArraySize(bytes_per_line, image->matte ? 4 : 3);
  if ((bytes_per_line == 0) || (bytes_per_line > 0x7FFFU) || ((row_bytes+MaxCount*2U) >= 0x7FFFU))
    ThrowPICTWriterException(CoderError,UnsupportedNumberOfColumns,image);
#if defined(PTRDIFF_MAX)
  /*
    Without this limit check we get the following warning from GCC when allocating row_bytes:

    warning: argument 1 value ‘18446744073709551488’ exceeds maximum object size 9223372036854775807 [-Walloc-size-larger-than=]
  */
  if (!(row_bytes <= PTRDIFF_MAX))
    ThrowPICTWriterException(ResourceLimitError,MemoryAllocationFailed,image);
#endif /* if defined(PTRDIFF_MAX) */
  buffer=MagickAllocateMemory(unsigned char *,PictInfoSize);
  packed_scanline=MagickAllocateMemory(unsigned char *,row_bytes+MaxCount*2U);
  scanline=MagickAllocateMemory(unsigned char *,row_bytes);
  if ((buffer == (unsigned char *) NULL) ||
      (packed_scanline == (unsigned char *) NULL) ||
      (scanline == (unsigned char *) NULL))
    ThrowPICTWriterException(ResourceLimitError,MemoryAllocationFailed,image);
  /*
    Write header, header size, size bounding box, version, and reserved.
  */
  (void) memset(buffer,0,PictInfoSize);
  (void) WriteBlob(image,PictInfoSize,buffer);
  (void) WriteBlobMSBShort(image,0);
  (void) WriteBlobMSBShort(image,size_rectangle.top);
  (void) WriteBlobMSBShort(image,size_rectangle.left);
  (void) WriteBlobMSBShort(image,size_rectangle.bottom);
  (void) WriteBlobMSBShort(image,size_rectangle.right);
  (void) WriteBlobMSBShort(image,PictVersion);
  (void) WriteBlobMSBShort(image,0x02ff);  /* version #2 */
  (void) WriteBlobMSBShort(image,PictInfoOp);
  (void) WriteBlobMSBLong(image,0xFFFE0000UL);
  /*
    Write full size of the file, resolution, frame bounding box, and reserved.
  */
  (void) WriteBlobMSBShort(image,(unsigned short) x_resolution);
  (void) WriteBlobMSBShort(image,0x0000);
  (void) WriteBlobMSBShort(image,(unsigned short) y_resolution);
  (void) WriteBlobMSBShort(image,0x0000);
  (void) WriteBlobMSBShort(image,frame_rectangle.top);
  (void) WriteBlobMSBShort(image,frame_rectangle.left);
  (void) WriteBlobMSBShort(image,frame_rectangle.bottom);
  (void) WriteBlobMSBShort(image,frame_rectangle.right);
  (void) WriteBlobMSBLong(image,0x00000000L);
  /*
    Output 8BIM profile.
  */
  profile_info=GetImageProfile(image,"8BIM",&profile_length);
  if (profile_info != (unsigned char *) NULL)
    {
      (void) WriteBlobMSBShort(image,0xa1);
      (void) WriteBlobMSBShort(image,0x1f2);
      (void) WriteBlobMSBShort(image,(const magick_uint16_t) profile_length+4);
      (void) WriteBlobString(image,"8BIM");
      (void) WriteBlob(image,profile_length,
                       profile_info);
    }
  /*
    Ouput ICM profile.
  */
  profile_info=GetImageProfile(image,"ICM",&profile_length);
  if (profile_info != (unsigned char *) NULL)
    {
      (void) WriteBlobMSBShort(image,0xa1);
      (void) WriteBlobMSBShort(image,0xe0);
      (void) WriteBlobMSBShort(image,(const magick_uint16_t) profile_length+4);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlob(image,profile_length,
                       profile_info);
      (void) WriteBlobMSBShort(image,0xa1);
      (void) WriteBlobMSBShort(image,0xe0);
      (void) WriteBlobMSBShort(image,4);
      (void) WriteBlobMSBLong(image,0x00000002UL);
    }
  /*
    Write crop region opcode and crop bounding box.
  */
  (void) WriteBlobMSBShort(image,PictCropRegionOp);
  (void) WriteBlobMSBShort(image,0xa);
  (void) WriteBlobMSBShort(image,crop_rectangle.top);
  (void) WriteBlobMSBShort(image,crop_rectangle.left);
  (void) WriteBlobMSBShort(image,crop_rectangle.bottom);
  (void) WriteBlobMSBShort(image,crop_rectangle.right);
  if (image->compression == JPEGCompression)
    {
      Image
        *jpeg_image;

      size_t
        length;

      unsigned char
        *blob;

      jpeg_image=CloneImage(image,0,0,True,&image->exception);
      if (jpeg_image == (Image *) NULL)
        {
          LiberatePICTAllocations();
          CloseBlob(image);
          return (False);
        }
      DestroyBlob(jpeg_image);
      jpeg_image->blob=CloneBlobInfo((BlobInfo *) NULL);
      (void) strcpy(jpeg_image->magick,"JPEG");
      blob=(unsigned char *) ImageToBlob(image_info,jpeg_image,&length,
        &image->exception);
      DestroyImage(jpeg_image);
      if (blob == (unsigned char *) NULL)
        {
          LiberatePICTAllocations();
          CloseBlob(image);
          return(False);
        }
      (void) WriteBlobMSBShort(image,PictJPEGOp);
      (void) WriteBlobMSBLong(image,(const magick_uint16_t) length+154);
      (void) WriteBlobMSBShort(image,0x0000);
      (void) WriteBlobMSBLong(image,0x00010000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00010000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x40000000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00400000UL);
      (void) WriteBlobMSBShort(image,0x0000);
      (void) WriteBlobMSBShort(image,image->rows);
      (void) WriteBlobMSBShort(image,image->columns);
      (void) WriteBlobMSBShort(image,0x0000);
      (void) WriteBlobMSBShort(image,768);
      (void) WriteBlobMSBShort(image,0x0000);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00566A70UL);
      (void) WriteBlobMSBLong(image,0x65670000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00000001UL);
      (void) WriteBlobMSBLong(image,0x00016170UL);
      (void) WriteBlobMSBLong(image,0x706C0000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBShort(image,768);
      (void) WriteBlobMSBShort(image,image->columns);
      (void) WriteBlobMSBShort(image,image->rows);
      (void) WriteBlobMSBShort(image,(unsigned short) x_resolution);
      (void) WriteBlobMSBShort(image,0x0000);
      (void) WriteBlobMSBShort(image,(unsigned short) y_resolution);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x87AC0001UL);
      (void) WriteBlobMSBLong(image,0x0B466F74UL);
      (void) WriteBlobMSBLong(image,0x6F202D20UL);
      (void) WriteBlobMSBLong(image,0x4A504547UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x00000000UL);
      (void) WriteBlobMSBLong(image,0x0018FFFFUL);
      (void) WriteBlob(image,length,blob);
      if (length & 0x01)
        (void) WriteBlobByte(image,'\0');
      MagickFreeMemory(blob);
    }
  /*
    Write picture opcode, row bytes, and picture bounding box, and version.
  */
  if (storage_class == PseudoClass)
    (void) WriteBlobMSBShort(image,PictPICTOp);
  else
    {
      (void) WriteBlobMSBShort(image,PictPixmapOp);
      (void) WriteBlobMSBLong(image,(unsigned long) base_address);
    }
  (void) WriteBlobMSBShort(image,(unsigned short) (row_bytes | 0x8000));
  (void) WriteBlobMSBShort(image,bounds.top);
  (void) WriteBlobMSBShort(image,bounds.left);
  (void) WriteBlobMSBShort(image,bounds.bottom);
  (void) WriteBlobMSBShort(image,bounds.right);
  /*
    Write pack type, pack size, resolution, pixel type, and pixel size.
  */
  (void) WriteBlobMSBShort(image,pixmap.version);
  (void) WriteBlobMSBShort(image,pixmap.pack_type);
  (void) WriteBlobMSBLong(image,pixmap.pack_size);
  (void) WriteBlobMSBShort(image,(unsigned short) x_resolution);
  (void) WriteBlobMSBShort(image,0x0000);
  (void) WriteBlobMSBShort(image,(unsigned short) y_resolution);
  (void) WriteBlobMSBShort(image,0x0000);
  (void) WriteBlobMSBShort(image,pixmap.pixel_type);
  (void) WriteBlobMSBShort(image,pixmap.bits_per_pixel);
  /*
    Write component count, size, plane bytes, table size, and reserved.
  */
  (void) WriteBlobMSBShort(image,pixmap.component_count);
  (void) WriteBlobMSBShort(image,pixmap.component_size);
  (void) WriteBlobMSBLong(image,(unsigned long) pixmap.plane_bytes);
  (void) WriteBlobMSBLong(image,(unsigned long) pixmap.table);
  (void) WriteBlobMSBLong(image,(unsigned long) pixmap.reserved);
  if (storage_class == PseudoClass)
    {
      /*
        Write image colormap.
      */
      (void) WriteBlobMSBLong(image,0x00000000L);  /* color seed */
      (void) WriteBlobMSBShort(image,0L);  /* color flags */
      (void) WriteBlobMSBShort(image,(unsigned short) (image->colors-1));
      for (i=0; i < (long) image->colors; i++)
      {
        (void) WriteBlobMSBShort(image,i);
        (void) WriteBlobMSBShort(image,
          ScaleQuantumToShort(image->colormap[i].red));
        (void) WriteBlobMSBShort(image,
          ScaleQuantumToShort(image->colormap[i].green));
        (void) WriteBlobMSBShort(image,
          ScaleQuantumToShort(image->colormap[i].blue));
      }
    }
  /*
    Write source and destination rectangle.
  */
  (void) WriteBlobMSBShort(image,source_rectangle.top);
  (void) WriteBlobMSBShort(image,source_rectangle.left);
  (void) WriteBlobMSBShort(image,source_rectangle.bottom);
  (void) WriteBlobMSBShort(image,source_rectangle.right);
  (void) WriteBlobMSBShort(image,destination_rectangle.top);
  (void) WriteBlobMSBShort(image,destination_rectangle.left);
  (void) WriteBlobMSBShort(image,destination_rectangle.bottom);
  (void) WriteBlobMSBShort(image,destination_rectangle.right);
  (void) WriteBlobMSBShort(image,transfer_mode);
  /*
    Write picture data.
  */
  count=0;
  if (storage_class == PseudoClass)
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=AccessImmutableIndexes(image);
      for (x=0; x < (long) image->columns; x++)
        scanline[x]=indexes[x];
      count+=EncodeImage(image,scanline,row_bytes & 0x7FFF,packed_scanline);
      if (QuantumTick(y,image->rows))
        if (!MagickMonitorFormatted(y,image->rows,&image->exception,
                                    SaveImageText,image->filename,
                                    image->columns,image->rows))
          break;
    }
  else
    if (image->compression == JPEGCompression)
      {
        (void) memset(scanline,0,row_bytes);
        for (y=0; y < (long) image->rows; y++)
          count+=EncodeImage(image,scanline,row_bytes & 0x7FFF,packed_scanline);
      }
    else
      {
        register unsigned char
          *blue,
          *green,
          *opacity,
          *red;

        red=scanline;
        green=scanline+image->columns;
        blue=scanline+ (size_t)2*image->columns;
        opacity=scanline+ (size_t)3*image->columns;
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          red=scanline;
          green=scanline+image->columns;
          blue=scanline+ (size_t)2*image->columns;
          if (image->matte)
            {
              opacity=scanline;
              red=scanline+image->columns;
              green=scanline+ (size_t)2*image->columns;
              blue=scanline+ (size_t)3*image->columns;
            }
          for (x=0; x < (long) image->columns; x++)
          {
            *red++=ScaleQuantumToChar(p->red);
            *green++=ScaleQuantumToChar(p->green);
            *blue++=ScaleQuantumToChar(p->blue);
            if (image->matte)
              *opacity++=ScaleQuantumToChar(MaxRGB-p->opacity);
            p++;
          }
          count+=EncodeImage(image,scanline,bytes_per_line & 0x7FFF,packed_scanline);
          if (QuantumTick(y,image->rows))
            if (!MagickMonitorFormatted(y,image->rows,&image->exception,
                                        SaveImageText,image->filename,
                                        image->columns,image->rows))
              break;
        }
      }
  if (count & 0x1)
    (void) WriteBlobByte(image,'\0');
  (void) WriteBlobMSBShort(image,PictEndOfPictureOp);
  offset=TellBlob(image);
  (void) SeekBlob(image,512,SEEK_SET);
  (void) WriteBlobMSBShort(image,(unsigned long) offset);
  MagickFreeMemory(scanline);
  MagickFreeMemory(packed_scanline);
  MagickFreeMemory(buffer);
  CloseBlob(image);
  return(True);
}
