/*
% Copyright (C) 2003 - 2020 GraphicsMagick Group
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
%                            BBBB   M   M  PPPP                               %
%                            B   B  MM MM  P   P                              %
%                            BBBB   M M M  PPPP                               %
%                            B   B  M   M  P                                  %
%                            BBBB   M   M  P                                  %
%                                                                             %
%                                                                             %
%             Read/Write Microsoft Windows Bitmap Image Format.               %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                            Glenn Randers-Pehrson                            %
%                               December 2001                                 %
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
#include "magick/constitute.h"
#include "magick/enum_strings.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/monitor.h"
#include "magick/pixel_cache.h"
#include "magick/profile.h"
#include "magick/transform.h"
#include "magick/utility.h"

/*
  Macro definitions (from Windows wingdi.h).
*/

#undef BI_JPEG
#define BI_JPEG  4
#undef BI_PNG
#define BI_PNG  5
#if !defined(MSWINDOWS) || defined(__MINGW32__)
#undef BI_RGB
#define BI_RGB  0
#undef BI_RLE8
#define BI_RLE8  1
#undef BI_RLE4
#define BI_RLE4  2
#undef BI_BITFIELDS
#define BI_BITFIELDS  3

#undef LCS_CALIBRATED_RBG
#define LCS_CALIBRATED_RBG  0
#undef LCS_sRGB
#define LCS_sRGB  1
#undef LCS_WINDOWS_COLOR_SPACE
#define LCS_WINDOWS_COLOR_SPACE  2
#undef PROFILE_LINKED
#define PROFILE_LINKED  3
#undef PROFILE_EMBEDDED
#define PROFILE_EMBEDDED  4

#undef LCS_GM_BUSINESS
#define LCS_GM_BUSINESS  1  /* Saturation */
#undef LCS_GM_GRAPHICS
#define LCS_GM_GRAPHICS  2  /* Relative */
#undef LCS_GM_IMAGES
#define LCS_GM_IMAGES  4  /* Perceptual */
#undef LCS_GM_ABS_COLORIMETRIC
#define LCS_GM_ABS_COLORIMETRIC  8  /* Absolute */
#endif

/*
  Typedef declarations.
*/
typedef struct _BMPInfo
{
  size_t
    file_size,  /* 0 or size of file in bytes */
    image_size; /* bytes_per_line*image->rows or uint32_t from file */

  magick_uint32_t
    ba_offset,
    offset_bits,/* Starting position of image data in bytes */
    size;       /* Header size 12 = v2, 12-64 OS/2 v2, 40 = v3, 108 = v4, 124 = v5 */

  magick_int32_t
    width,      /* BMP width */
    height;     /* BMP height (negative means bottom-up) */

  magick_uint16_t
    planes,
    bits_per_pixel;

  magick_uint32_t
    compression,
    x_pixels,
    y_pixels,
    number_colors,
    colors_important;

  magick_uint32_t
    red_mask,
    green_mask,
    blue_mask,
    alpha_mask;

  magick_int32_t
    colorspace;

  PrimaryInfo
    red_primary,
    green_primary,
    blue_primary,
    gamma_scale;
} BMPInfo;

/*
  Forward declarations.
*/
static unsigned int
  WriteBMPImage(const ImageInfo *,Image *);

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
%  Method DecodeImage unpacks the packed image pixels into runlength-encoded
%  pixel packets.
%
%  The format of the DecodeImage method is:
%
%      MagickPassFail DecodeImage(Image *image,const unsigned long compression,
%        unsigned char *pixels)
%
%  A description of each parameter follows:
%
%    o status:  Method DecodeImage returns MagickPass if all the pixels are
%      uncompressed without error, otherwise MagickFail.
%
%    o image: The address of a structure of type Image.
%
%    o compression:  Zero means uncompressed.  A value of 1 means the
%      compressed pixels are runlength encoded for a 256-color bitmap.
%      A value of 2 means a 16-color bitmap.  A value of 3 means bitfields
%      encoding.
%
%    o pixels:  The address of a byte (8 bits) array of pixel data created by
%      the decoding process.
%
%    o pixels_size: The size of the allocated buffer array.
%
*/
static MagickPassFail DecodeImage(Image *image,const unsigned long compression,
                                  unsigned char *pixels, const size_t pixels_size)
{
  unsigned long
    x,
    y;

  unsigned int
    i;

  int
    byte,
    count;

  register unsigned char
    *q;

  unsigned char
    *end;

  assert(image != (Image *) NULL);
  assert(pixels != (unsigned char *) NULL);
  if (image->logging)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                          "  Decoding RLE compressed pixels to"
                          " %" MAGICK_SIZE_T_F "u bytes",
                          (MAGICK_SIZE_T ) image->rows*image->columns);

  (void) memset(pixels,0,pixels_size);
  byte=0;
  x=0;
  q=pixels;
  end=pixels + pixels_size;
  /*
    Decompress sufficient data to support the number of pixels (or
    rows) in the image and then return.

    Do not wait to read the final EOL and EOI markers (if not yet
    encountered) since we always read this marker just before we
    return.
  */
  for (y=0; y < image->rows; )
    {
      if (q < pixels || q >= end)
        {
          if (image->logging)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                  "  Decode buffer full (y=%lu, "
                                  "pixels_size=%" MAGICK_SIZE_T_F "u, "
                                  "pixels=%p, q=%p, end=%p)",
                                  y, (MAGICK_SIZE_T) pixels_size,
                                  pixels, q, end);
          break;
        }
      count=ReadBlobByte(image);
      if (count == EOF)
        return MagickFail;
      if (count > 0)
        {
          count=Min(count, end - q);
          /*
            Encoded mode.
          */
          byte=ReadBlobByte(image);
          if (byte == EOF)
            return MagickFail;
          if (compression == BI_RLE8)
            {
              for ( i=count; i != 0; --i )
                {
                  *q++=(unsigned char) byte;
                }
            }
          else
            {
              for ( i=0; i < (unsigned int) count; i++ )
                {
                  *q++=(unsigned char)
                    ((i & 0x01) ? (byte & 0x0f) : ((byte >> 4) & 0x0f));
                }
            }
          x+=count;
        }
      else
        {
          /*
            Escape mode.
          */
          count=ReadBlobByte(image);
          if (count == EOF)
            return MagickFail;
          if (count == 0x01)
            {
              if (image->logging)
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                      "  RLE Escape code encountered");
              goto rle_decode_done;
            }
          switch (count)
            {
            case 0x00:
              {
                /*
                  End of line.
                */
                x=0;
                y++;
                q=pixels+y*image->columns;
                break;
              }
            case 0x02:
              {
                /*
                  Delta mode.
                */
                byte=ReadBlobByte(image);
                if (byte == EOF)
                  return MagickFail;
                x+=byte;
                byte=ReadBlobByte(image);
                if (byte == EOF)
                  return MagickFail;
                y+=byte;
                q=pixels+y*image->columns+x;
                break;
              }
            default:
              {
                /*
                  Absolute mode.
                */
                count=Min(count, end - q);
                if (count < 0)
                  return MagickFail;
                if (compression == BI_RLE8)
                  for (i=count; i != 0; --i)
                    {
                      byte=ReadBlobByte(image);
                      if (byte == EOF)
                        return MagickFail;
                      *q++=byte;
                    }
                else
                  for (i=0; i < (unsigned int) count; i++)
                    {
                      if ((i & 0x01) == 0)
                        {
                          byte=ReadBlobByte(image);
                          if (byte == EOF)
                            return MagickFail;
                        }
                      *q++=(unsigned char)
                        ((i & 0x01) ? (byte & 0x0f) : ((byte >> 4) & 0x0f));
                    }
                x+=count;
                /*
                  Read pad byte.
                */
                if (compression == BI_RLE8)
                  {
                    if (count & 0x01)
                      if (ReadBlobByte(image) == EOF)
                        return MagickFail;
                  }
                else
                  if (((count & 0x03) == 1) || ((count & 0x03) == 2))
                    if (ReadBlobByte(image) == EOF)
                      return MagickFail;
                break;
              }
            }
        }
      if (QuantumTick(y,image->rows))
        if (!MagickMonitorFormatted(y,image->rows,&image->exception,
                                    LoadImageText,image->filename,
                                    image->columns,image->rows))
          break;
    }
  (void) ReadBlobByte(image);  /* end of line */
  (void) ReadBlobByte(image);
 rle_decode_done:
  if (image->logging)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                          "  Decoded %" MAGICK_SIZE_T_F "u bytes",
                          (MAGICK_SIZE_T) (q-pixels));
  if ((MAGICK_SIZE_T) (q-pixels) < pixels_size)
    {
      if (image->logging)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                              "  RLE decoded output is truncated");
      return MagickFail;
    }
  return(MagickPass);
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
%  Method EncodeImage compresses pixels using a runlength encoded format.
%
%  The format of the EncodeImage method is:
%
%    static unsigned int EncodeImage(Image *image,
%      const unsigned long bytes_per_line,const unsigned char *pixels,
%      unsigned char *compressed_pixels)
%
%  A description of each parameter follows:
%
%    o status:  Method EncodeImage returns the number of bytes in the
%      runlength encoded compress_pixels array.
%
%    o image:  A pointer to an Image structure.
%
%    o bytes_per_line: The number of bytes in a scanline of compressed pixels
%
%    o pixels:  The address of a byte (8 bits) array of pixel data created by
%      the compression process.
%
%    o compressed_pixels:  The address of a byte (8 bits) array of compressed
%      pixel data.
%
%
*/
static size_t EncodeImage(Image *image,const size_t bytes_per_line,
  const unsigned char *pixels,unsigned char *compressed_pixels)
{
  unsigned long
    y;

  register const unsigned char
    *p;

  register unsigned long
    i,
    x;

  register unsigned char
    *q;

  /*
    Runlength encode pixels.
  */
  assert(image != (Image *) NULL);
  assert(pixels != (const unsigned char *) NULL);
  assert(compressed_pixels != (unsigned char *) NULL);
  p=pixels;
  q=compressed_pixels;
  i=0;
  for (y=0; y < image->rows; y++)
  {
    for (x=0; x < bytes_per_line; x+=i)
    {
      /*
        Determine runlength.
      */
      for (i=1; ((x+i) < bytes_per_line); i++)
        if ((i == 255) || (*(p+i) != *p))
          break;
      *q++=(unsigned char) i;
      *q++=(*p);
      p+=i;
    }
    /*
      End of line.
    */
    *q++=0x00;
    *q++=0x00;
    if (QuantumTick(y,image->rows))
      if (!MagickMonitorFormatted(y,image->rows,&image->exception,
                                  SaveImageText,image->filename,
                                  image->columns,image->rows))
        break;
  }
  /*
    End of bitmap.
  */
  *q++=0;
  *q++=0x01;
  return((size_t) (q-compressed_pixels));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s B M P                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method IsBMP returns True if the image format type, identified by the
%  magick string, is BMP.
%
%  The format of the IsBMP method is:
%
%      unsigned int IsBMP(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o status:  Method IsBMP returns True if the image format type is BMP.
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static unsigned int IsBMP(const unsigned char *magick,const size_t length)
{
  if (length < 2)
    return(False);
  if ((LocaleNCompare((char *) magick,"BA",2) == 0) ||
      (LocaleNCompare((char *) magick,"BM",2) == 0) ||
      (LocaleNCompare((char *) magick,"IC",2) == 0) ||
      (LocaleNCompare((char *) magick,"PI",2) == 0) ||
      (LocaleNCompare((char *) magick,"CI",2) == 0) ||
      (LocaleNCompare((char *) magick,"CP",2) == 0))
    return(True);
  return(False);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d B M P I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadBMPImage reads a Microsoft Windows bitmap image file, Version
%  2, 3 (for Windows or NT), or 4, and  returns it.  It allocates the memory
%  necessary for the new Image structure and returns a pointer to the new
%  image.
%
%  The format of the ReadBMPImage method is:
%
%      image=ReadBMPImage(image_info)
%
%  A description of each parameter follows:
%
%    o image:  Method ReadBMPImage returns a pointer to the image after
%      reading.  A null image is returned if there is a memory shortage or
%      if the image cannot be read.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
#define ThrowBMPReaderException(code_,reason_,image_) \
do { \
  MagickFreeMemory(bmp_colormap); \
  MagickFreeMemory(pixels); \
  ThrowReaderException(code_,reason_,image_); \
} while (0);

static Image *ReadBMPImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  BMPInfo
    bmp_info;

  Image
    *image;

  int
    logging;

  long
    y;

  magick_uint32_t
    blue,
    green,
    opacity,
    red;

  ExtendedSignedIntegralType
    start_position;

  register long
    x;

  register PixelPacket
    *q;

  register long
    i;

  register unsigned char
    *p;

  size_t
    bytes_per_line,
    count,
    length,
    pixels_size;

  unsigned char
    *bmp_colormap,
    magick[12],
    *pixels;

  unsigned int
    status;

  magick_off_t
    file_remaining,
    file_size,
    offset;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  bmp_colormap=(unsigned char *) NULL;
  pixels=(unsigned char *) NULL;
  logging=LogMagickEvent(CoderEvent,GetMagickModule(),"enter");
  image=AllocateImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == False)
    ThrowBMPReaderException(FileOpenError,UnableToOpenFile,image);
  file_size=GetBlobSize(image);
  /*
    Determine if this is a BMP file.
  */
  (void) memset(&bmp_info,0,sizeof(BMPInfo));
  bmp_info.ba_offset=0;
  start_position=0;
  magick[0]=magick[1]=0;
  count=ReadBlob(image,2,(char *) magick);
  do
  {
    PixelPacket
      quantum_bits,
      shift;

    unsigned long
      profile_data,
      profile_size;

    /*
      Verify BMP identifier.
    */
    /* if (bmp_info.ba_offset == 0) */ /* FIXME: Investigate. Start position needs to always advance! */
    start_position=TellBlob(image)-2;
    bmp_info.ba_offset=0;
     /* "BA" is OS/2 bitmap array file */
    while (LocaleNCompare((char *) magick,"BA",2) == 0)
    {
      bmp_info.file_size=ReadBlobLSBLong(image);
      bmp_info.ba_offset=ReadBlobLSBLong(image);
      bmp_info.offset_bits=ReadBlobLSBLong(image);
      if ((count=ReadBlob(image,2,(char *) magick)) != 2)
        break;
    }
    if (logging && count == 2)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  Magick: %c%c",
        magick[0],magick[1]);
    if ((count != 2) || /* Found "BA" header from above above */
        ((LocaleNCompare((char *) magick,"BM",2) != 0) && /* "BM" is Windows or OS/2 file. */
         (LocaleNCompare((char *) magick,"CI",2) != 0)))  /* "CI" is OS/2 Color Icon */
      ThrowBMPReaderException(CorruptImageError,ImproperImageHeader,image);
    bmp_info.file_size=ReadBlobLSBLong(image); /* File size in bytes */
    if (logging)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "  File size: Claimed=%" MAGICK_SIZE_T_F "u, Actual=%"
                            MAGICK_OFF_F "d",
                            (MAGICK_SIZE_T) bmp_info.file_size, file_size);
    (void) ReadBlobLSBLong(image); /* Reserved */
    bmp_info.offset_bits=ReadBlobLSBLong(image); /* Bit map offset from start of file */
    bmp_info.size=ReadBlobLSBLong(image);  /* BMP Header size */
    if (logging)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "  Header size: %u\n"
                            "    Offset bits: %u\n"
                            "    Image data offset: %u",
                            bmp_info.size,
                            bmp_info.offset_bits,
                            bmp_info.ba_offset);

    if ((bmp_info.file_size != 0) && ((magick_off_t) bmp_info.file_size > file_size))
      ThrowBMPReaderException(CorruptImageError,ImproperImageHeader,image);
    if ((bmp_info.size != 12) && (bmp_info.size != 40) && (bmp_info.size != 108)
        && (bmp_info.size != 124) &&
        (!(bmp_info.size >= 12 && bmp_info.size <= 64)))
      ThrowBMPReaderException(CorruptImageError,ImproperImageHeader,image);
    if (bmp_info.offset_bits < bmp_info.size)
      ThrowBMPReaderException(CorruptImageError,ImproperImageHeader,image);

    if (bmp_info.size == 12)
      {
        /*
          Windows 2.X or OS/2 BMP image file.
        */
        bmp_info.width=(magick_int16_t) ReadBlobLSBShort(image); /* Width */
        bmp_info.height=(magick_int16_t) ReadBlobLSBShort(image); /* Height */
        bmp_info.planes=ReadBlobLSBShort(image); /* # of color planes */
        bmp_info.bits_per_pixel=ReadBlobLSBShort(image); /* Bits per pixel */
        bmp_info.x_pixels=0;
        bmp_info.y_pixels=0;
        bmp_info.number_colors=0;
        bmp_info.compression=BI_RGB;
        bmp_info.image_size=0;
        bmp_info.alpha_mask=0;
        if (logging)
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Format: Windows 2.X or OS/2 Bitmap");
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Geometry: %dx%d",bmp_info.width,bmp_info.height);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Planes: %u",bmp_info.planes);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Bits per pixel: %u",bmp_info.bits_per_pixel);
          }
      }
    else
      {
        /*
          Microsoft Windows 3.X or later BMP image file.
        */
        if (bmp_info.size < 40)
          ThrowBMPReaderException(CorruptImageError,NonOS2HeaderSizeError,
            image);

        /*
          BMP v3 defines width and hight as signed LONG (32 bit) values.  If
          height is a positive number, then the image is a "bottom-up"
          bitmap with origin in the lower-left corner.  If height is a
          negative number, then the image is a "top-down" bitmap with the
          origin in the upper-left corner.  The meaning of negative values
          is not defined for width.
        */
        bmp_info.width=(magick_int32_t) ReadBlobLSBLong(image); /* Width */
        bmp_info.height=(magick_int32_t) ReadBlobLSBLong(image); /* Height */
        bmp_info.planes=ReadBlobLSBShort(image); /* # of color planes */
        bmp_info.bits_per_pixel=ReadBlobLSBShort(image); /* Bits per pixel (1/4/8/16/24/32) */
        bmp_info.compression=ReadBlobLSBLong(image); /* Compression method */
        bmp_info.image_size=ReadBlobLSBLong(image); /* Bitmap size (bytes) */
        bmp_info.x_pixels=ReadBlobLSBLong(image); /* Horizontal resolution (pixels/meter) */
        bmp_info.y_pixels=ReadBlobLSBLong(image); /* Vertical resolution (pixels/meter) */
        bmp_info.number_colors=ReadBlobLSBLong(image); /* Number of colors */
        bmp_info.colors_important=ReadBlobLSBLong(image); /* Minimum important colors */
        profile_data=0;
        profile_size=0;
        if (logging)
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Format: MS Windows bitmap 3.X");
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Geometry: %dx%d",bmp_info.width,bmp_info.height);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Planes: %u",bmp_info.planes);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Bits per pixel: %u",bmp_info.bits_per_pixel);
            switch ((int) bmp_info.compression)
            {
              case BI_RGB:
              {
                /* uncompressed */
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_RGB");
                break;
              }
              case BI_RLE4:
              {
                /* 4 bit RLE */
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_RLE4");
                break;
              }
              case BI_RLE8:
              {
                /* 8 bit RLE */
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_RLE8");
                break;
              }
              case BI_BITFIELDS:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_BITFIELDS");
                break;
              }
              case BI_PNG:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_PNG");
                break;
              }
              case BI_JPEG:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: BI_JPEG");
                break;
              }
              default:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                  "  Compression: UNKNOWN (%u)",bmp_info.compression);
              }
            }
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Number of colors: %u",bmp_info.number_colors);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Important colors: %u",bmp_info.colors_important);
          }

        bmp_info.red_mask=ReadBlobLSBLong(image);
        bmp_info.green_mask=ReadBlobLSBLong(image);
        bmp_info.blue_mask=ReadBlobLSBLong(image);

        if (bmp_info.size > 40)
          {
            double
              sum;

            /*
              Read color management information.
            */
            bmp_info.alpha_mask=ReadBlobLSBLong(image);
            bmp_info.colorspace=(magick_int32_t) ReadBlobLSBLong(image);
            /*
              Decode 2^30 fixed point formatted CIE primaries.
            */
            bmp_info.red_primary.x=(double)
              ReadBlobLSBLong(image)/0x3ffffff;
            bmp_info.red_primary.y=(double)
              ReadBlobLSBLong(image)/0x3ffffff;
            bmp_info.red_primary.z=(double)
              ReadBlobLSBLong(image)/0x3ffffff;
            bmp_info.green_primary.x=(double)
              ReadBlobLSBLong(image)/0x3ffffff;
            bmp_info.green_primary.y=(double)
              ReadBlobLSBLong(image)/0x3ffffff;
            bmp_info.green_primary.z=(double)
              ReadBlobLSBLong(image)/0x3ffffff;
            bmp_info.blue_primary.x=(double)
              ReadBlobLSBLong(image)/0x3ffffff;
            bmp_info.blue_primary.y=(double)
              ReadBlobLSBLong(image)/0x3ffffff;
            bmp_info.blue_primary.z=(double)
              ReadBlobLSBLong(image)/0x3ffffff;

            sum=bmp_info.red_primary.x+bmp_info.red_primary.y+
              bmp_info.red_primary.z;
            sum=Max(MagickEpsilon,sum);
            bmp_info.red_primary.x/=sum;
            bmp_info.red_primary.y/=sum;
            image->chromaticity.red_primary.x=bmp_info.red_primary.x;
            image->chromaticity.red_primary.y=bmp_info.red_primary.y;

            sum=bmp_info.green_primary.x+bmp_info.green_primary.y+
              bmp_info.green_primary.z;
            sum=Max(MagickEpsilon,sum);
            bmp_info.green_primary.x/=sum;
            bmp_info.green_primary.y/=sum;
            image->chromaticity.green_primary.x=bmp_info.green_primary.x;
            image->chromaticity.green_primary.y=bmp_info.green_primary.y;

            sum=bmp_info.blue_primary.x+bmp_info.blue_primary.y+
              bmp_info.blue_primary.z;
            sum=Max(MagickEpsilon,sum);
            bmp_info.blue_primary.x/=sum;
            bmp_info.blue_primary.y/=sum;
            image->chromaticity.blue_primary.x=bmp_info.blue_primary.x;
            image->chromaticity.blue_primary.y=bmp_info.blue_primary.y;

            /*
              Decode 16^16 fixed point formatted gamma_scales.
            */
            bmp_info.gamma_scale.x=(double) ReadBlobLSBLong(image)/0xffff;
            bmp_info.gamma_scale.y=(double) ReadBlobLSBLong(image)/0xffff;
            bmp_info.gamma_scale.z=(double) ReadBlobLSBLong(image)/0xffff;
            /*
              Compute a single gamma from the BMP 3-channel gamma.
            */
            image->gamma=(bmp_info.gamma_scale.x+bmp_info.gamma_scale.y+
              bmp_info.gamma_scale.z)/3.0;
          }
        if (bmp_info.size > 108)
          {
            unsigned long
              intent;

            /*
              Read BMP Version 5 color management information.
            */
            intent=ReadBlobLSBLong(image);
            switch ((int) intent)
            {
              case LCS_GM_BUSINESS:
              {
                image->rendering_intent=SaturationIntent;
                break;
              }
              case LCS_GM_GRAPHICS:
              {
                image->rendering_intent=RelativeIntent;
                break;
              }
              case LCS_GM_IMAGES:
              {
                image->rendering_intent=PerceptualIntent;
                break;
              }
              case LCS_GM_ABS_COLORIMETRIC:
              {
                image->rendering_intent=AbsoluteIntent;
                break;
              }
            }
            profile_data=ReadBlobLSBLong(image);
            profile_size=ReadBlobLSBLong(image);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                  "  Profile: size %lu data %lu",
                                  profile_size,profile_data);
            (void) ReadBlobLSBLong(image);  /* Reserved byte */
          }
      }

    if (logging)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "  File size: Claimed=%" MAGICK_SIZE_T_F "u, Actual=%"
                            MAGICK_OFF_F "d",
                            (MAGICK_SIZE_T) bmp_info.file_size, file_size);
    /*
      It seems that some BMPs claim a file size two bytes larger than
      they actually are so allow some slop before warning about file
      size.
    */
    if ((magick_off_t) bmp_info.file_size > file_size+2)
      {
        ThrowException(exception,CorruptImageWarning,
                       LengthAndFilesizeDoNotMatch,image->filename);
      }
    if (logging && (magick_off_t) bmp_info.file_size < file_size)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                              "  Discarding all data beyond bmp_info.file_size");
    if (bmp_info.width <= 0)
      ThrowBMPReaderException(CorruptImageError,NegativeOrZeroImageSize,image);
    if ((bmp_info.height) == 0 || (bmp_info.height < -2147483647))
      ThrowBMPReaderException(CorruptImageError,NegativeOrZeroImageSize,image);
    if ((bmp_info.height < 0) && (bmp_info.compression !=0))
      ThrowBMPReaderException(CorruptImageError,CompressionNotValid,image);
    if (bmp_info.planes != 1)
      ThrowBMPReaderException(CorruptImageError,StaticPlanesValueNotEqualToOne,
        image);
    if ((bmp_info.bits_per_pixel != 1) && (bmp_info.bits_per_pixel != 4) &&
        (bmp_info.bits_per_pixel != 8) && (bmp_info.bits_per_pixel != 16) &&
        (bmp_info.bits_per_pixel != 24) && (bmp_info.bits_per_pixel != 32))
      ThrowBMPReaderException(CorruptImageError,UnrecognizedBitsPerPixel,image);
    if (bmp_info.bits_per_pixel < 16)
      {
        if (bmp_info.number_colors > (1UL << bmp_info.bits_per_pixel))
          ThrowBMPReaderException(CorruptImageError,UnrecognizedNumberOfColors,image);
      }
    if (bmp_info.compression > 3)
      ThrowBMPReaderException(CorruptImageError,UnrecognizedImageCompression,image);
    if ((bmp_info.compression == 1) && (bmp_info.bits_per_pixel != 8))
      ThrowBMPReaderException(CorruptImageError,UnrecognizedBitsPerPixel,image);
    if ((bmp_info.compression == 2) && (bmp_info.bits_per_pixel != 4))
      ThrowBMPReaderException(CorruptImageError,UnrecognizedBitsPerPixel,image);
    if ((bmp_info.compression == 3) && (bmp_info.bits_per_pixel < 16))
      ThrowBMPReaderException(CorruptImageError,UnrecognizedBitsPerPixel,image);
    switch ((unsigned int) bmp_info.compression)
    {
      case BI_RGB:
      case BI_RLE8:
      case BI_RLE4:
      case BI_BITFIELDS:
        break;
      case BI_JPEG:
        ThrowBMPReaderException(CoderError,JPEGCompressionNotSupported,image)
      case BI_PNG:
        ThrowBMPReaderException(CoderError,PNGCompressionNotSupported,image)
      default:
        ThrowBMPReaderException(CorruptImageError,UnrecognizedImageCompression,
          image)
    }
    image->columns=bmp_info.width;
    image->rows=AbsoluteValue(bmp_info.height);
    image->depth=8;
    /*
      Image has alpha channel if alpha mask is specified, or is
      uncompressed and 32-bits per pixel
    */
    image->matte=((bmp_info.alpha_mask != 0)
                  || ((bmp_info.compression == BI_RGB)
                      && (bmp_info.bits_per_pixel == 32)));
    if (bmp_info.bits_per_pixel < 16)
      {
        if (bmp_info.number_colors == 0)
          image->colors=1L << bmp_info.bits_per_pixel;
        else
          image->colors=bmp_info.number_colors;
        image->storage_class=PseudoClass;
      }
    if (image->storage_class == PseudoClass)
      {
        unsigned int
          packet_size;

        /*
          Read BMP raster colormap.
        */
        if (logging)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "  Reading colormap of %u colors",image->colors);
        if (!AllocateImageColormap(image,image->colors))
          ThrowBMPReaderException(ResourceLimitError,MemoryAllocationFailed,
            image);
        bmp_colormap=MagickAllocateArray(unsigned char *,4,image->colors);
        if (bmp_colormap == (unsigned char *) NULL)
          ThrowBMPReaderException(ResourceLimitError,MemoryAllocationFailed,
            image);
        if ((bmp_info.size == 12) || (bmp_info.size == 64))
          packet_size=3;
        else
          packet_size=4;
        offset=start_position+14+bmp_info.size;
        if (logging)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                "Seek offset %" MAGICK_OFF_F "d",
                                (magick_off_t) offset);
        if ((offset < start_position) ||
            (SeekBlob(image,offset,SEEK_SET) != (magick_off_t) offset))
          ThrowBMPReaderException(CorruptImageError,ImproperImageHeader,image);
        if (ReadBlob(image,packet_size*image->colors,(char *) bmp_colormap)
            != (size_t) packet_size*image->colors)
          ThrowBMPReaderException(CorruptImageError,UnexpectedEndOfFile,image);
        p=bmp_colormap;
        for (i=0; i < (long) image->colors; i++)
        {
          image->colormap[i].blue=ScaleCharToQuantum(*p++);
          image->colormap[i].green=ScaleCharToQuantum(*p++);
          image->colormap[i].red=ScaleCharToQuantum(*p++);
          if (packet_size == 4)
            p++;
        }
        MagickFreeMemory(bmp_colormap);
      }

    if (image_info->ping && (image_info->subrange != 0))
        if (image->scene >= (image_info->subimage+image_info->subrange-1))
          break;

    if (CheckImagePixelLimits(image, exception) != MagickPass)
        ThrowBMPReaderException(ResourceLimitError,ImagePixelLimitExceeded,image);

    /*
      Read image data.
    */
    if (logging)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "start_position %" MAGICK_OFF_F "d,"
                            " bmp_info.offset_bits %" MAGICK_OFF_F "d,"
                            " bmp_info.ba_offset %" MAGICK_OFF_F "d" ,
                            (magick_off_t) start_position,
                            (magick_off_t) bmp_info.offset_bits,
                            (magick_off_t) bmp_info.ba_offset);
    offset=start_position+bmp_info.offset_bits;
    if (logging)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "Seek offset %" MAGICK_OFF_F "d",
                            (magick_off_t) offset);
    if ((offset < start_position) ||
        (SeekBlob(image,offset,SEEK_SET) != (magick_off_t) offset))
      ThrowBMPReaderException(CorruptImageError,ImproperImageHeader,image);
    if (bmp_info.compression == BI_RLE4)
      bmp_info.bits_per_pixel<<=1;
    if (logging)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "image->columns: %lu, bmp_info.bits_per_pixel %u",
                            image->columns, bmp_info.bits_per_pixel);
    /*
      Below emulates:
      bytes_per_line=4*((image->columns*bmp_info.bits_per_pixel+31)/32);
    */
    bytes_per_line=MagickArraySize(image->columns,bmp_info.bits_per_pixel);
    if ((bytes_per_line > 0) && (~((size_t) 0) - bytes_per_line) > 31)
      bytes_per_line = MagickArraySize(4,(bytes_per_line+31)/32);
    if (bytes_per_line == 0)
      ThrowBMPReaderException(CoderError,ArithmeticOverflow,image);

    if (logging)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "  Bytes per line: %" MAGICK_SIZE_T_F "u",
                            (MAGICK_SIZE_T) bytes_per_line);

    length=MagickArraySize(bytes_per_line,image->rows);
    if (logging)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "  Expected total raster length: %" MAGICK_SIZE_T_F "u",
                            (MAGICK_SIZE_T) length);
    if (length == 0)
      ThrowBMPReaderException(CoderError,ArithmeticOverflow,image);

    /*
      Check that file data is reasonable given claims by file header.
      We do this before allocating raster memory to avoid DOS.
    */
    if ((bmp_info.compression == BI_RGB) ||
        (bmp_info.compression == BI_BITFIELDS))
      {
        /*
          Not compressed.
        */
        file_remaining=file_size-TellBlob(image);
        if (file_remaining < (magick_off_t) length)
          ThrowBMPReaderException(CorruptImageError,InsufficientImageDataInFile,
                                  image);
      }
    else if ((bmp_info.compression == BI_RLE4) ||
             (bmp_info.compression == BI_RLE8))
      {
        /* RLE Compressed.  Assume a maximum compression ratio. */
        file_remaining=file_size-TellBlob(image);
        if ((file_remaining <= 0) || (((double) length/file_remaining) > 254.0))
          ThrowBMPReaderException(CorruptImageError,InsufficientImageDataInFile,
                                  image);
      }

    if (~((size_t) 0) - image->columns < 1)
      ThrowBMPReaderException(CoderError,ArithmeticOverflow,image);
    pixels_size=MagickArraySize(Max(bytes_per_line,(size_t) image->columns+1),
                                image->rows);
    if (logging)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "  Pixels size %" MAGICK_SIZE_T_F "u",
                            (MAGICK_SIZE_T) pixels_size);
    if (pixels_size == 0)
      ThrowBMPReaderException(CoderError,ArithmeticOverflow,image);
    pixels=MagickAllocateMemory(unsigned char *, pixels_size);
    if (pixels == (unsigned char *) NULL)
      ThrowBMPReaderException(ResourceLimitError,MemoryAllocationFailed,image);
    if ((bmp_info.compression == BI_RGB) ||
        (bmp_info.compression == BI_BITFIELDS))
      {
        if (logging)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "  Reading pixels (%" MAGICK_SIZE_T_F "u bytes)",
                                (MAGICK_SIZE_T) length);
        if (ReadBlob(image,length,(char *) pixels) != (size_t) length)
          ThrowBMPReaderException(CorruptImageError,UnexpectedEndOfFile,image);
      }
    else
      {
        /*
          Convert run-length encoded raster pixels.

          DecodeImage() normally decompresses to rows*columns bytes of data.
        */
        status=DecodeImage(image,bmp_info.compression,pixels,
                           image->rows*image->columns);
        if (status == MagickFail)
          ThrowBMPReaderException(CorruptImageError,UnableToRunlengthDecodeImage,
            image);
      }
    /*
      Initialize image structure.
    */
    image->units=PixelsPerCentimeterResolution;
    image->x_resolution=bmp_info.x_pixels/100.0;
    image->y_resolution=bmp_info.y_pixels/100.0;
    /*
      Convert BMP raster image to pixel packets.
    */
    if (bmp_info.compression == BI_RGB)
      {
        bmp_info.alpha_mask=(image->matte ? 0xff000000U : 0U);
        bmp_info.red_mask=0x00ff0000U;
        bmp_info.green_mask=0x0000ff00U;
        bmp_info.blue_mask=0x000000ffU;
        if (bmp_info.bits_per_pixel == 16)
          {
            /*
              RGB555.
            */
            bmp_info.red_mask=0x00007c00U;
            bmp_info.green_mask=0x000003e0U;
            bmp_info.blue_mask=0x0000001fU;
          }
      }
    if ((bmp_info.bits_per_pixel == 16) || (bmp_info.bits_per_pixel == 32))
      {
        register magick_uint32_t
          sample;

        /*
          Get shift and quantum bits info from bitfield masks.
        */
        (void) memset(&shift,0,sizeof(PixelPacket));
        (void) memset(&quantum_bits,0,sizeof(PixelPacket));
        if (bmp_info.red_mask != 0U)
          while ((shift.red < 32U) && (((bmp_info.red_mask << shift.red) & 0x80000000U) == 0))
            shift.red++;
        if (bmp_info.green_mask != 0)
          while ((shift.green < 32U) && (((bmp_info.green_mask << shift.green) & 0x80000000U) == 0))
            shift.green++;
        if (bmp_info.blue_mask != 0)
          while ((shift.blue < 32U) && (((bmp_info.blue_mask << shift.blue) & 0x80000000U) == 0))
            shift.blue++;
        if (bmp_info.alpha_mask != 0)
          while ((shift.opacity < 32U) && (((bmp_info.alpha_mask << shift.opacity) & 0x80000000U) == 0))
            shift.opacity++;
        sample=shift.red;
        while ((sample < 32U) && (((bmp_info.red_mask << sample) & 0x80000000U) != 0))
          sample++;
        quantum_bits.red=(Quantum) (sample-shift.red);
        sample=shift.green;
        while ((sample < 32U) && (((bmp_info.green_mask << sample) & 0x80000000U) != 0))
          sample++;
        quantum_bits.green=(Quantum) (sample-shift.green);
        sample=shift.blue;
        while ((sample < 32U) && (((bmp_info.blue_mask << sample) & 0x80000000U) != 0))
          sample++;
        quantum_bits.blue=(Quantum) (sample-shift.blue);
        sample=shift.opacity;
        while ((sample < 32U) && (((bmp_info.alpha_mask << sample) & 0x80000000U) != 0))
          sample++;
        quantum_bits.opacity=(Quantum) (sample-shift.opacity);
      }
    switch (bmp_info.bits_per_pixel)
    {
      case 1:
      {
        /*
          Convert bitmap scanline.
        */
        for (y=(long) image->rows-1; y >= 0; y--)
        {
          p=pixels+(image->rows-y-1)*bytes_per_line;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          if (ImportImagePixelArea(image,IndexQuantum,bmp_info.bits_per_pixel,p,0,0)
              == MagickFail)
            break;
          if (!SyncImagePixels(image))
            break;
          if (image->previous == (Image *) NULL)
            if (QuantumTick(y,image->rows))
              {
                status=MagickMonitorFormatted(image->rows-y-1,image->rows,
                                              exception,LoadImageText,
                                              image->filename,
                                              image->columns,image->rows);
                if (status == False)
                  break;
              }
        }
        break;
      }
      case 4:
      {
        /*
          Convert PseudoColor scanline.
        */
        for (y=(long) image->rows-1; y >= 0; y--)
        {
          p=pixels+(image->rows-y-1)*bytes_per_line;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          if (ImportImagePixelArea(image,IndexQuantum,bmp_info.bits_per_pixel,p,0,0)
              == MagickFail)
            break;
          if (!SyncImagePixels(image))
            break;
          if (image->previous == (Image *) NULL)
            if (QuantumTick(y,image->rows))
              {
                status=MagickMonitorFormatted(image->rows-y-1,image->rows,
                                              exception,LoadImageText,
                                              image->filename,
                                              image->columns,image->rows);
                if (status == False)
                  break;
              }
        }
        break;
      }
      case 8:
      {
        /*
          Convert PseudoColor scanline.
        */
        if ((bmp_info.compression == BI_RLE8) ||
            (bmp_info.compression == BI_RLE4))
          bytes_per_line=image->columns;
        for (y=(long) image->rows-1; y >= 0; y--)
        {
          p=pixels+(image->rows-y-1)*bytes_per_line;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          if (ImportImagePixelArea(image,IndexQuantum,bmp_info.bits_per_pixel,p,0,0)
              == MagickFail)
            break;
          if (!SyncImagePixels(image))
            break;
          if (image->previous == (Image *) NULL)
            if (QuantumTick(y,image->rows))
              {
                status=MagickMonitorFormatted(image->rows-y-1,image->rows,
                                              exception,LoadImageText,
                                              image->filename,
                                              image->columns,image->rows);
                if (status == False)
                  break;
              }
        }
        break;
      }
      case 16:
      {
        magick_uint32_t
          pixel;

        /*
          Convert bitfield encoded 16-bit PseudoColor scanline.
        */
        if (bmp_info.compression != BI_RGB &&
            bmp_info.compression != BI_BITFIELDS)
          ThrowBMPReaderException(CorruptImageError,UnrecognizedImageCompression,image)
        bytes_per_line=2*(image->columns+image->columns%2);
        image->storage_class=DirectClass;
        for (y=(long) image->rows-1; y >= 0; y--)
        {
          p=pixels+(image->rows-y-1)*bytes_per_line;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            pixel=(*p++);
            pixel|=(*p++) << 8;
            red=((pixel & bmp_info.red_mask) << shift.red) >> 16;
            if (quantum_bits.red == 8)
              red|=(red >> 8);
            green=((pixel & bmp_info.green_mask) << shift.green) >> 16;
            if (quantum_bits.green == 8)
              green|=(green >> 8);
            blue=((pixel & bmp_info.blue_mask) << shift.blue) >> 16;
            if (quantum_bits.blue == 8)
              blue|=(blue >> 8);
            if (image->matte != False)
              {
                opacity=((pixel & bmp_info.alpha_mask) << shift.opacity) >> 16;
                if (quantum_bits.opacity == 8)
                  opacity|=(opacity >> 8);
                q->opacity=MaxRGB-ScaleShortToQuantum(opacity);
              }
            q->red=ScaleShortToQuantum(red);
            q->green=ScaleShortToQuantum(green);
            q->blue=ScaleShortToQuantum(blue);
            q++;
          }
          if (!SyncImagePixels(image))
            break;
          if (image->previous == (Image *) NULL)
            if (QuantumTick(y,image->rows))
              {
                status=MagickMonitorFormatted(image->rows-y-1,image->rows,
                                              exception,LoadImageText,
                                              image->filename,
                                              image->columns,image->rows);
                if (status == False)
                  break;
              }
        }
        break;
      }
      case 24:
      {
        /*
          Convert DirectColor scanline.
        */
        bytes_per_line=4*((image->columns*24+31)/32);
        for (y=(long) image->rows-1; y >= 0; y--)
        {
          p=pixels+(image->rows-y-1)*bytes_per_line;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            q->blue=ScaleCharToQuantum(*p++);
            q->green=ScaleCharToQuantum(*p++);
            q->red=ScaleCharToQuantum(*p++);
            q++;
          }
          if (!SyncImagePixels(image))
            break;
          if (image->previous == (Image *) NULL)
            if (QuantumTick(y,image->rows))
              {
                status=MagickMonitorFormatted(image->rows-y-1,image->rows,
                                              exception,LoadImageText,
                                              image->filename,
                                              image->columns,image->rows);
                if (status == False)
                  break;
              }
        }
        break;
      }
      case 32:
      {
        /*
          Convert bitfield encoded DirectColor scanline.
        */
        if ((bmp_info.compression != BI_RGB) &&
            (bmp_info.compression != BI_BITFIELDS))
          ThrowBMPReaderException(CorruptImageError,UnrecognizedImageCompression,image)
        bytes_per_line=4*(image->columns);
        for (y=(long) image->rows-1; y >= 0; y--)
        {
          magick_uint32_t
            pixel;

          p=pixels+(image->rows-y-1)*bytes_per_line;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            pixel =((magick_uint32_t) *p++);
            pixel|=((magick_uint32_t) *p++ << 8);
            pixel|=((magick_uint32_t) *p++ << 16);
            pixel|=((magick_uint32_t) *p++ << 24);
            red=((pixel & bmp_info.red_mask) << shift.red) >> 16;
            if (quantum_bits.red == 8)
              red|=(red >> 8);
            green=((pixel & bmp_info.green_mask) << shift.green) >> 16;
            if (quantum_bits.green == 8)
              green|=(green >> 8);
            blue=((pixel & bmp_info.blue_mask) << shift.blue) >> 16;
            if (quantum_bits.blue == 8)
              blue|=(blue >> 8);
            if (image->matte != False)
              {
                opacity=((pixel & bmp_info.alpha_mask) << shift.opacity) >> 16;
                if (quantum_bits.opacity == 8)
                  opacity|=(opacity >> 8);
                q->opacity=MaxRGB-ScaleShortToQuantum(opacity);
              }
            q->red=ScaleShortToQuantum(red);
            q->green=ScaleShortToQuantum(green);
            q->blue=ScaleShortToQuantum(blue);
            q++;
          }
          if (!SyncImagePixels(image))
            break;
          if (image->previous == (Image *) NULL)
            if (QuantumTick(y,image->rows))
              {
                status=MagickMonitorFormatted(image->rows-y-1,image->rows,
                                              exception,LoadImageText,
                                              image->filename,
                                              image->columns,image->rows);
                if (status == False)
                  break;
              }
        }
        break;
      }
      default:
        ThrowBMPReaderException(CorruptImageError,ImproperImageHeader,image)
    }
    MagickFreeMemory(pixels);
    if (EOFBlob(image))
      {
        ThrowException(exception,CorruptImageError,UnexpectedEndOfFile,
          image->filename);
        break;
      }
    if (bmp_info.height < 0)
      {
        Image
          *flipped_image;

        /*
          Correct image orientation.
        */
        flipped_image=FlipImage(image,exception);
        if (flipped_image == (Image *) NULL)
          {
            DestroyImageList(image);
            return((Image *) NULL);
          }
        DestroyBlob(flipped_image);
        flipped_image->blob=ReferenceBlob(image->blob);
        ReplaceImageInList(&image,flipped_image);
      }
    StopTimer(&image->timer);
    /*
      Proceed to next image.
    */
    if (image_info->subrange != 0)
      if (image->scene >= (image_info->subimage+image_info->subrange-1))
        break;
    *magick='\0';
    file_remaining=file_size-TellBlob(image);
    if (file_remaining == 0)
      break;
    offset=bmp_info.ba_offset;
    if (logging)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "Seek offset %" MAGICK_OFF_F "d",
                            (magick_off_t) offset);
    if (offset > 0)
      if ((offset < TellBlob(image)) ||
          (SeekBlob(image,offset,SEEK_SET) != (magick_off_t) offset))
      ThrowBMPReaderException(CorruptImageError,ImproperImageHeader,image);
    if (ReadBlob(image,2,(char *) magick) != (size_t) 2)
      break;
    if (IsBMP(magick,2))
      {
        /*
          Acquire next image structure.
        */
        AllocateNextImage(image_info,image);
        if (image->next == (Image *) NULL)
          {
            DestroyImageList(image);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
        status=MagickMonitorFormatted(TellBlob(image),GetBlobSize(image),
                                      exception,LoadImagesText,
                                      image->filename);
        if (status == False)
          break;
      }
  } while (IsBMP(magick,2));
  while (image->previous != (Image *) NULL)
    image=image->previous;
  CloseBlob(image);
  if (logging)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"return");
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r B M P I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RegisterBMPImage adds attributes for the BMP image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterBMPImage method is:
%
%      RegisterBMPImage(void)
%
*/
ModuleExport void RegisterBMPImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("BMP");
  entry->decoder=(DecoderHandler) ReadBMPImage;
  entry->encoder=(EncoderHandler) WriteBMPImage;
  entry->magick=(MagickHandler) IsBMP;
  entry->description="Microsoft Windows bitmap image";
  entry->module="BMP";
  entry->adjoin=False;
  entry->seekable_stream=True;
  entry->coder_class=PrimaryCoderClass;
  (void) RegisterMagickInfo(entry);

  entry=SetMagickInfo("BMP2");
  entry->encoder=(EncoderHandler) WriteBMPImage;
  entry->magick=(MagickHandler) IsBMP;
  entry->description="Microsoft Windows bitmap image v2";
  entry->module="BMP";
  entry->adjoin=False;
  entry->coder_class=PrimaryCoderClass;
  entry->seekable_stream=True;
  (void) RegisterMagickInfo(entry);

  entry=SetMagickInfo("BMP3");
  entry->encoder=(EncoderHandler) WriteBMPImage;
  entry->magick=(MagickHandler) IsBMP;
  entry->description="Microsoft Windows bitmap image v3";
  entry->module="BMP";
  entry->adjoin=False;
  entry->seekable_stream=True;
  entry->coder_class=PrimaryCoderClass;
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r B M P I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method UnregisterBMPImage removes format registrations made by the
%  BMP module from the list of supported formats.
%
%  The format of the UnregisterBMPImage method is:
%
%      UnregisterBMPImage(void)
%
*/
ModuleExport void UnregisterBMPImage(void)
{
  (void) UnregisterMagickInfo("BMP");
  (void) UnregisterMagickInfo("BMP2");
  (void) UnregisterMagickInfo("BMP3");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e B M P I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WriteBMPImage writes an image in Microsoft Windows bitmap encoded
%  image format, version 3 for Windows or (if the image has a matte channel)
%  version 4.
%
%  The format of the WriteBMPImage method is:
%
%      unsigned int WriteBMPImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o status: Method WriteBMPImage return True if the image is written.
%      False is returned is there is a memory shortage or if the image file
%      fails to write.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o image:  A pointer to an Image structure.
%
%
*/
static unsigned int WriteBMPImage(const ImageInfo *image_info,Image *image)
{
  BMPInfo
    bmp_info;

  int
    logging;

  unsigned long
    y;

  register const PixelPacket
    *p;

  register unsigned long
    i,
    x;

  register unsigned char
    *q;

  unsigned char
    *bmp_data,
    *pixels;

  size_t
    bytes_per_line,
    image_size;

  MagickBool
    adjoin,
    have_color_info;

  MagickPassFail
    status;

  unsigned long
    scene,
    type;

  /*   const unsigned char */
  /*     *color_profile=0; */

  size_t
    color_profile_length=0;

  size_t
    image_list_length;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  image_list_length=GetImageListLength(image);
  logging=LogMagickEvent(CoderEvent,GetMagickModule(),"enter");
  if (logging)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                          "%" MAGICK_SIZE_T_F "u image frames in list",
                          (MAGICK_SIZE_T) image_list_length);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFail)
    ThrowWriterException(FileOpenError,UnableToOpenFile,image);
  type=4;
  if (LocaleCompare(image_info->magick,"BMP2") == 0)
    type=2;
  else
    if (LocaleCompare(image_info->magick,"BMP3") == 0)
      type=3;
  scene=0;
  adjoin=image_info->adjoin;

  /*
    Retrieve color profile from Image (if any)
    FIXME: is color profile support writing not properly implemented?
  */
  /* color_profile= */ (void) GetImageProfile(image,"ICM",&color_profile_length);

  do
    {
      /*
        Initialize BMP raster file header.
      */
      if (logging)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                              "Original: Scene %lu, storage_class %s, colors %u",
                              scene,
                              ClassTypeToString(image->storage_class),
                              image->colors);
      (void) TransformColorspace(image,RGBColorspace);
      (void) memset(&bmp_info,0,sizeof(BMPInfo));
      bmp_info.file_size=14+12;
      if (type > 2)
        bmp_info.file_size+=28;
      bmp_info.offset_bits=(magick_uint32_t) bmp_info.file_size;
      bmp_info.compression=BI_RGB;
      if ((image->storage_class != DirectClass) && (image->colors > 256))
        (void) SetImageType(image,TrueColorType);
      if (image->storage_class != DirectClass)
        {
          /*
            Colormapped BMP raster.
          */
          bmp_info.bits_per_pixel=8;
          if (image->colors <= 2)
            bmp_info.bits_per_pixel=1;
          else if (image->colors <= 16)
            bmp_info.bits_per_pixel=4;
          else if (image->colors <= 256)
            bmp_info.bits_per_pixel=8;
          bmp_info.number_colors=1 << bmp_info.bits_per_pixel;
          if (image->matte)
            (void) SetImageType(image,TrueColorMatteType);
          else
            if (bmp_info.number_colors < image->colors)
              (void) SetImageType(image,TrueColorType);
            else
              {
                bmp_info.file_size+=3U*(1U << bmp_info.bits_per_pixel);
                bmp_info.offset_bits+=3U*(1U << bmp_info.bits_per_pixel);
                if (type > 2)
                  {
                    bmp_info.file_size+=(size_t) (1U << bmp_info.bits_per_pixel);
                    bmp_info.offset_bits+=(1U << bmp_info.bits_per_pixel);
                  }
              }
        }
      if (image->storage_class == DirectClass)
        {
          /*
            Full color BMP raster.
          */
          bmp_info.number_colors=0;
          bmp_info.bits_per_pixel=((type > 3) && image->matte) ? 32 : 24;
          bmp_info.compression=
            (type > 3) && image->matte ?  BI_BITFIELDS : BI_RGB;
        }
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                            "Final: Scene %lu, storage_class %s, colors %u",
                            scene,
                            ClassTypeToString(image->storage_class),
                            image->colors);
      /*
        Below emulates:
        bytes_per_line=4*((image->columns*bmp_info.bits_per_pixel+31)/32);
      */
      bytes_per_line=MagickArraySize(image->columns,bmp_info.bits_per_pixel);
      if ((bytes_per_line > 0) && (~((size_t) 0) - bytes_per_line) > 31)
        bytes_per_line = MagickArraySize(4,(bytes_per_line+31)/32);
      if (bytes_per_line == 0)
        ThrowWriterException(CoderError,ArithmeticOverflow,image);
      image_size=MagickArraySize(bytes_per_line,image->rows);
      if ((image_size == 0) || ((image_size & 0xffffffff) != image_size))
        ThrowWriterException(CoderError,ArithmeticOverflow,image);
      bmp_info.ba_offset=0;
      have_color_info=(int) ((image->rendering_intent != UndefinedIntent) ||
                             (color_profile_length != 0) || (image->gamma != 0.0));
      if (type == 2)
        bmp_info.size=12;
      else
        if ((type == 3) || (!image->matte && !have_color_info))
          {
            type=3;
            bmp_info.size=40;
          }
        else
          {
            int
              extra_size;

            bmp_info.size=108;
            extra_size=68;
            if ((image->rendering_intent != UndefinedIntent) ||
                (color_profile_length != 0))
              {
                bmp_info.size=124;
                extra_size+=16;
              }
            bmp_info.file_size+=extra_size;
            bmp_info.offset_bits+=extra_size;
          }
      /*
        Verify and enforce that image dimensions do not exceed limit
        imposed by file format.
      */
      if (type == 2)
        {
          bmp_info.width=(magick_int16_t) image->columns;
          bmp_info.height=(magick_int16_t) image->rows;
        }
      else
        {
          bmp_info.width=(magick_int32_t) image->columns;
          bmp_info.height=(magick_int32_t) image->rows;
        }
      if (((unsigned long) bmp_info.width != image->columns) ||
          ((unsigned long) bmp_info.height != image->rows))
        {
          ThrowWriterException(CoderError,ImageColumnOrRowSizeIsNotSupported,image);
        }

      bmp_info.planes=1;
      bmp_info.image_size=image_size;
      bmp_info.file_size+=bmp_info.image_size;
      bmp_info.x_pixels=75*39;
      bmp_info.y_pixels=75*39;
      if (image->units == PixelsPerInchResolution)
        {
          bmp_info.x_pixels=(unsigned long) (100.0*image->x_resolution/2.54);
          bmp_info.y_pixels=(unsigned long) (100.0*image->y_resolution/2.54);
        }
      if (image->units == PixelsPerCentimeterResolution)
        {
          bmp_info.x_pixels=(unsigned long) (100.0*image->x_resolution);
          bmp_info.y_pixels=(unsigned long) (100.0*image->y_resolution);
        }
      bmp_info.colors_important=bmp_info.number_colors;
      /*
        Convert MIFF to BMP raster pixels.
      */
      pixels=MagickAllocateMemory(unsigned char *,bmp_info.image_size);
      if (pixels == (unsigned char *) NULL)
        ThrowWriterException(ResourceLimitError,MemoryAllocationFailed,image);
      switch (bmp_info.bits_per_pixel)
        {
        case 1:
          {
            ExportPixelAreaOptions
              export_options;

            /*
              Convert PseudoClass image to a BMP monochrome image.
            */
            if (logging)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                    "  Output %u-bit PseudoClass pixels",
                                    bmp_info.bits_per_pixel);
            ExportPixelAreaOptionsInit(&export_options);
            export_options.pad_bytes=(unsigned long) (bytes_per_line - ((image->columns+7)/8));
            export_options.pad_value=0x00;
            for (y=0; y < image->rows; y++)
              {
                p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                q=pixels+(image->rows-y-1)*bytes_per_line;
                if (ExportImagePixelArea(image,IndexQuantum,1,q,&export_options,0)
                    == MagickFail)
                  {
                    break;
                  }
                if (image->previous == (Image *) NULL)
                  if (QuantumTick(y,image->rows))
                    if (!MagickMonitorFormatted(y,image->rows,&image->exception,
                                                SaveImageText,image->filename,
                                                image->columns,image->rows))
                      break;
              }
            break;
          }
        case 4:
          {
            ExportPixelAreaOptions
              export_options;

            /*
              Convert PseudoClass image to a BMP monochrome image.
            */
            if (logging)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                    "  Output %u-bit PseudoClass pixels",
                                    bmp_info.bits_per_pixel);
            ExportPixelAreaOptionsInit(&export_options);
            export_options.pad_bytes=(unsigned long) (bytes_per_line - ((image->columns+1)/2));
            export_options.pad_value=0x00;
            for (y=0; y < image->rows; y++)
              {
                p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                q=pixels+(image->rows-y-1)*bytes_per_line;
                if (ExportImagePixelArea(image,IndexQuantum,4,q,&export_options,0)
                    == MagickFail)
                  {
                    break;
                  }
                if (image->previous == (Image *) NULL)
                  if (QuantumTick(y,image->rows))
                    if (!MagickMonitorFormatted(y,image->rows,&image->exception,
                                                SaveImageText,image->filename,
                                                image->columns,image->rows))
                      break;
              }
            break;
          }
        case 8:
          {
            ExportPixelAreaOptions
              export_options;

            /*
              Convert PseudoClass packet to BMP pixel.
            */
            if (logging)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                    "  Output %u-bit PseudoClass pixels",
                                    bmp_info.bits_per_pixel);
            ExportPixelAreaOptionsInit(&export_options);
            export_options.pad_bytes=(unsigned long) (bytes_per_line - image->columns);
            for (y=0; y < image->rows; y++)
              {
                p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                q=pixels+(image->rows-y-1)*bytes_per_line;
                if (ExportImagePixelArea(image,IndexQuantum,8,q,&export_options,0)
                    == MagickFail)
                  {
                    break;
                  }
                if (image->previous == (Image *) NULL)
                  if (QuantumTick(y,image->rows))
                    if (!MagickMonitorFormatted(y,image->rows,&image->exception,
                                                SaveImageText,image->filename,
                                                image->columns,image->rows))
                      break;
              }
            break;
          }
        case 24:
        case 32:
          {
            /*
              Convert DirectClass packet to BMP BGR888 or BGRA8888 pixel.
            */
            if (logging)
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                    "  Output %u-bit DirectClass pixels",
                                    bmp_info.bits_per_pixel);
            for (y=0; y < image->rows; y++)
              {
                p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                q=pixels+(image->rows-y-1)*bytes_per_line;
                for (x=0; x < image->columns; x++)
                  {
                    *q++=ScaleQuantumToChar(p->blue);
                    *q++=ScaleQuantumToChar(p->green);
                    *q++=ScaleQuantumToChar(p->red);
                    if (bmp_info.bits_per_pixel == 32)
                      *q++=ScaleQuantumToChar(MaxRGB-p->opacity);
                    p++;
                  }
                if (bmp_info.bits_per_pixel == 24)
                  {
                    /* initialize padding bytes */
                    for (x=3*image->columns; x < bytes_per_line; x++)
                      *q++=0x00;
                  }
                if (image->previous == (Image *) NULL)
                  if (QuantumTick(y,image->rows))
                    if (!MagickMonitorFormatted(y,image->rows,&image->exception,
                                                SaveImageText,image->filename,
                                                image->columns,image->rows))
                      break;
              }
            break;
          }
        default:
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                  "Unsupported bits-per-pixel %u!",
                                  bmp_info.bits_per_pixel);

            break;
          }
        }
      if ((type > 2) && (bmp_info.bits_per_pixel == 8))
        if (image_info->compression != NoCompression)
          {
            size_t
              length;

            /*
              Convert run-length encoded raster pixels.
            */
            length=2*(bytes_per_line+2)*(image->rows+2)+2;
            bmp_data=MagickAllocateMemory(unsigned char *,length);
            if (bmp_data == (unsigned char *) NULL)
              {
                MagickFreeMemory(pixels);
                ThrowWriterException(ResourceLimitError,MemoryAllocationFailed,
                                     image)
                  }
            bmp_info.file_size-=bmp_info.image_size;
            bmp_info.image_size=EncodeImage(image,bytes_per_line,pixels,
                                            bmp_data);
            bmp_info.file_size+=bmp_info.image_size;
            MagickFreeMemory(pixels);
            pixels=bmp_data;
            bmp_info.compression=BI_RLE8;
          }
      /*
        Write BMP for Windows, all versions, 14-byte header.
      */
      if (logging)
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                "   Writing BMP version %ld datastream",type);
          if (image->storage_class == DirectClass)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                  "   Storage class=DirectClass");
          else
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                  "   Storage class=PseudoClass");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                "   Image depth=%u",image->depth);
          if (image->matte)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                  "   Matte=True");
          else
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                  "   Matte=False");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                "   BMP bits_per_pixel=%d",bmp_info.bits_per_pixel);
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                "   BMP file_size=%" MAGICK_SIZE_T_F "u bytes",
                                (MAGICK_SIZE_T) bmp_info.file_size);
          switch (bmp_info.compression)
            {
            case BI_RGB:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                      "   Compression=BI_RGB");
                break;
              }
            case BI_RLE8:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                      "   Compression=BI_RLE8");
                break;
              }
            case BI_BITFIELDS:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                      "   Compression=BI_BITFIELDS");
                break;
              }
            default:
              {
                (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                      "   Compression=UNKNOWN (%u)",bmp_info.compression);
                break;
              }
            }
          if (bmp_info.number_colors == 0)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                  "   Number_colors=unspecified");
          else
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                  "   Number_colors=%u",bmp_info.number_colors);
        }
      (void) WriteBlob(image,2,"BM");
      (void) WriteBlobLSBLong(image,(magick_uint32_t) bmp_info.file_size);
      (void) WriteBlobLSBLong(image,bmp_info.ba_offset);  /* always 0 */
      (void) WriteBlobLSBLong(image,bmp_info.offset_bits);
      if (type == 2)
        {
          /*
            Write 12-byte version 2 bitmap header.
          */
          (void) WriteBlobLSBLong(image,bmp_info.size);
          (void) WriteBlobLSBShort(image,bmp_info.width);
          (void) WriteBlobLSBShort(image,bmp_info.height);
          (void) WriteBlobLSBShort(image,bmp_info.planes);
          (void) WriteBlobLSBShort(image,bmp_info.bits_per_pixel);
        }
      else
        {
          /*
            Write 40-byte version 3+ bitmap header.
          */
          (void) WriteBlobLSBLong(image,bmp_info.size);
          (void) WriteBlobLSBLong(image,bmp_info.width);
          (void) WriteBlobLSBLong(image,bmp_info.height);
          (void) WriteBlobLSBShort(image,bmp_info.planes);
          (void) WriteBlobLSBShort(image,bmp_info.bits_per_pixel);
          (void) WriteBlobLSBLong(image,bmp_info.compression);
          (void) WriteBlobLSBLong(image,(magick_uint32_t) bmp_info.image_size);
          (void) WriteBlobLSBLong(image,bmp_info.x_pixels);
          (void) WriteBlobLSBLong(image,bmp_info.y_pixels);
          (void) WriteBlobLSBLong(image,bmp_info.number_colors);
          (void) WriteBlobLSBLong(image,bmp_info.colors_important);
        }
      if ((type > 3) && (image->matte || have_color_info))
        {
          /*
            Write the rest of the 108-byte BMP Version 4 header.
          */
          (void) WriteBlobLSBLong(image,0x00ff0000L);  /* Red mask */
          (void) WriteBlobLSBLong(image,0x0000ff00L);  /* Green mask */
          (void) WriteBlobLSBLong(image,0x000000ffL);  /* Blue mask */
          (void) WriteBlobLSBLong(image,0xff000000UL);  /* Alpha mask */
          (void) WriteBlobLSBLong(image,0x00000001L);   /* CSType==Calib. RGB */
          (void) WriteBlobLSBLong(image,
                                  (magick_uint32_t) (image->chromaticity.red_primary.x*0x3ffffff));
          (void) WriteBlobLSBLong(image,
                                  (magick_uint32_t) (image->chromaticity.red_primary.y*0x3ffffff));
          (void) WriteBlobLSBLong(image,
                                  (magick_uint32_t) (1.000f-(image->chromaticity.red_primary.x
                                                             +image->chromaticity.red_primary.y)*0x3ffffff));
          (void) WriteBlobLSBLong(image,
                                  (magick_uint32_t) (image->chromaticity.green_primary.x*0x3ffffff));
          (void) WriteBlobLSBLong(image,
                                  (magick_uint32_t) (image->chromaticity.green_primary.y*0x3ffffff));
          (void) WriteBlobLSBLong(image,
                                  (magick_uint32_t) (1.000f-(image->chromaticity.green_primary.x
                                                             +image->chromaticity.green_primary.y)*0x3ffffff));
          (void) WriteBlobLSBLong(image,
                                  (magick_uint32_t) (image->chromaticity.blue_primary.x*0x3ffffff));
          (void) WriteBlobLSBLong(image,
                                  (magick_uint32_t) (image->chromaticity.blue_primary.y*0x3ffffff));
          (void) WriteBlobLSBLong(image,
                                  (magick_uint32_t) (1.000f-(image->chromaticity.blue_primary.x
                                                             +image->chromaticity.blue_primary.y)*0x3ffffff));

          (void) WriteBlobLSBLong(image,(magick_uint32_t) (bmp_info.gamma_scale.x*0xffff));
          (void) WriteBlobLSBLong(image,(magick_uint32_t) (bmp_info.gamma_scale.y*0xffff));
          (void) WriteBlobLSBLong(image,(magick_uint32_t) (bmp_info.gamma_scale.z*0xffff));
          if ((image->rendering_intent != UndefinedIntent) ||
              (color_profile_length != 0))
            {
              long
                intent;

              switch ((int) image->rendering_intent)
                {
                case SaturationIntent:
                  {
                    intent=LCS_GM_BUSINESS;
                    break;
                  }
                case RelativeIntent:
                  {
                    intent=LCS_GM_GRAPHICS;
                    break;
                  }
                case PerceptualIntent:
                  {
                    intent=LCS_GM_IMAGES;
                    break;
                  }
                case AbsoluteIntent:
                  {
                    intent=LCS_GM_ABS_COLORIMETRIC;
                    break;
                  }
                default:
                  {
                    intent=0;
                    break;
                  }
                }
              (void) WriteBlobLSBLong(image,intent);
              (void) WriteBlobLSBLong(image,0x0);  /* dummy profile data */
              (void) WriteBlobLSBLong(image,0x0);  /* dummy profile length */
              (void) WriteBlobLSBLong(image,0x0);  /* reserved */
            }
        }
      if (image->storage_class == PseudoClass)
        {
          unsigned char
            *bmp_colormap;

          /*
            Dump colormap to file.
          */
          if (logging)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                  "  Colormap: %u entries",image->colors);
          bmp_colormap=MagickAllocateArray(unsigned char *,4,
                                           (size_t) (1L << bmp_info.bits_per_pixel));
          if (bmp_colormap == (unsigned char *) NULL)
            {
              MagickFreeMemory(pixels);
              ThrowWriterException(ResourceLimitError,MemoryAllocationFailed,
                                   image);
            }
          q=bmp_colormap;
          for (i=0; i < Min(image->colors,bmp_info.number_colors); i++)
            {
              *q++=ScaleQuantumToChar(image->colormap[i].blue);
              *q++=ScaleQuantumToChar(image->colormap[i].green);
              *q++=ScaleQuantumToChar(image->colormap[i].red);
              if (type > 2)
                *q++=(Quantum) 0x0;
            }
          for ( ; i < (1UL << bmp_info.bits_per_pixel); i++)
            {
              *q++=(Quantum) 0x0;
              *q++=(Quantum) 0x0;
              *q++=(Quantum) 0x0;
              if (type > 2)
                *q++=(Quantum) 0x0;
            }
          if (type <= 2)
            (void) WriteBlob(image,3*(1UL << bmp_info.bits_per_pixel),
                             (char *) bmp_colormap);
          else
            (void) WriteBlob(image,4*(1UL << bmp_info.bits_per_pixel),
                             (char *) bmp_colormap);
          MagickFreeMemory(bmp_colormap);
        }
      if (logging)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                              "  Pixels:  %" MAGICK_SIZE_T_F "u bytes",
                              (MAGICK_SIZE_T) bmp_info.image_size);
      (void) WriteBlob(image,bmp_info.image_size,(char *) pixels);
      MagickFreeMemory(pixels);
      if (image->next == (Image *) NULL)
        {
          if (logging)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                                  "No more image frames in list (scene=%lu)",
                                  scene);
          break;
        }
      image=SyncNextImageInList(image);
      status&=MagickMonitorFormatted(scene++,image_list_length,
                                     &image->exception,SaveImagesText,
                                     image->filename);
      if (status != MagickPass)
        break;
      if (logging)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                              "At end of image adjoin loop (adjoin=%u, scene=%lu)",
                              image_info->adjoin, scene);
    } while (adjoin);
  if (adjoin)
    while (image->previous != (Image *) NULL)
      image=image->previous;
  CloseBlob(image);
  if (logging)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"return");
  return(True);
}
