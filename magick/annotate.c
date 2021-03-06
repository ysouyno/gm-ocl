/*
% Copyright (C) 2003 - 2018 GraphicsMagick Group
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
%           AAA   N   N  N   N   OOO   TTTTT   AAA   TTTTT  EEEEE             %
%          A   A  NN  N  NN  N  O   O    T    A   A    T    E                 %
%          AAAAA  N N N  N N N  O   O    T    AAAAA    T    EEE               %
%          A   A  N  NN  N  NN  O   O    T    A   A    T    E                 %
%          A   A  N   N  N   N   OOO     T    A   A    T    EEEEE             %
%                                                                             %
%                                                                             %
%                  GraphicsMagick Image Annotation Methods                    %
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
% Digital Applications (www.digapp.com) contributed the stroked text algorithm.
% It was written by Leonard Rosenthol.
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/alpha_composite.h"
#include "magick/analyze.h"
#include "magick/color.h"
#include "magick/color_lookup.h"
#include "magick/composite.h"
#include "magick/constitute.h"
#include "magick/gem.h"
#include "magick/log.h"
#include "magick/pixel_cache.h"
#include "magick/render.h"
#include "magick/tempfile.h"
#include "magick/transform.h"
#include "magick/utility.h"
#include "magick/xwindow.h"
#if defined(HasTTF)

#  if defined(__MINGW32__)
#    undef interface  /* Remove interface define */
#  endif

#  if defined(HAVE_FT2BUILD_H)
     /*
       Modern FreeType2 installs require that <ft2build.h> be included
       before including other FreeType2 headers.  Including
       <ft2build.h> establishes definitions used by other FreeType2
       headers.
     */
#    include <ft2build.h>
#    include FT_FREETYPE_H
#    include FT_GLYPH_H
#    include FT_OUTLINE_H
#    include FT_BBOX_H
#  else
     /*
       Very old way to include FreeType2
     */
#    include <freetype/freetype.h>
#    include <freetype/ftglyph.h>
#    include <freetype/ftoutln.h>
#    include <freetype/ftbbox.h>
#  endif /* defined(HAVE_FT2BUILD_H) */

#endif /* defined(HasTTF) */

/*
  Forward declarations.
*/
typedef magick_int32_t magick_code_point_t;

static unsigned int
  RenderType(Image *,const DrawInfo *,const PointInfo *,TypeMetric *),
  RenderPostscript(Image *,const DrawInfo *,const PointInfo *,TypeMetric *),
  RenderFreetype(Image *,const DrawInfo *,const char *,const PointInfo *,
    TypeMetric *),
  RenderX11(Image *,const DrawInfo *,const PointInfo *,TypeMetric *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A n n o t a t e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AnnotateImage() annotates an image with DrawInfo 'text' based on other
%  parameters from DrawInfo such as 'affine', 'align', 'decorate', and
%  'gravity'.
%
%  Originally this function additionally transformed 'text' using
%  TranslateText() but it no longer does so as of GraphicsMagick 1.3.32.
%
%  The format of the AnnotateImage method is:
%
%      MagickPassFail AnnotateImage(Image *image,DrawInfo *draw_info)
%
%  A description of each parameter follows:
%
%    o status: Method AnnotateImage returns MagickPass if the image is annotated
%      otherwise MagickFail.
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%
*/
MagickExport MagickPassFail AnnotateImage(Image *image,const DrawInfo *draw_info)
{
  char
    primitive[MaxTextExtent],
    *p,
    *text,
    **textlist;

  DrawInfo
    *annotate,
    *clone_info;

  PointInfo
    offset;

  RectangleInfo
    geometry;

  register size_t
    i;

  TypeMetric
    metrics;

  unsigned int
    matte;

  MagickPassFail
    status=MagickPass;

  unsigned long
    height,
    number_lines;

  MagickBool
    metrics_initialized = MagickFalse;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickSignature);
  if (draw_info->text == (char *) NULL)
    return(MagickFail);
  if (*draw_info->text == '\0')
    return(MagickPass);
  annotate=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  text=annotate->text;
  annotate->text=(char *) NULL;
  clone_info=CloneDrawInfo((ImageInfo *) NULL,annotate);
  /*
    Split text into list based on new-lines
  */
  number_lines=1;
  for (p=text; *p != '\0'; p++)
    if (*p == '\n')
      number_lines++;
  textlist=MagickAllocateArray(char **,((size_t) number_lines+1),sizeof(char *));
  if (textlist == (char **) NULL)
    MagickFatalError3(ResourceLimitFatalError,MemoryAllocationFailed,
                      UnableToConvertText);
  p=text;
  for (i=0; i < number_lines; i++)
    {
      char *q;
      textlist[i]=p;
      for (q=(char *) p; *q != '\0'; q++)
          if ((*q == '\r') || (*q == '\n'))
            break;
      if (*q == '\r')
        {
          *q='\0';
          q++;
        }
      *q='\0';
      p=q+1;
    }
  textlist[i]=(char *) NULL;

  SetGeometry(image,&geometry);
  if (draw_info->geometry != (char *) NULL)
    (void) GetGeometry(draw_info->geometry,&geometry.x,&geometry.y,
      &geometry.width,&geometry.height);
  matte=image->matte;
  status=MagickPass;
  for (i=0; textlist[i] != (char *) NULL; i++)
  {
    if (*textlist[i] == '\0')
      continue;
    /*
      Position text relative to image.
    */
    (void) CloneString(&annotate->text,textlist[i]);
    if ((!metrics_initialized) || (annotate->gravity != NorthWestGravity))
      {
        metrics_initialized=MagickTrue;
        (void) GetTypeMetrics(image,annotate,&metrics);
      }
    height=(unsigned long) (metrics.ascent-metrics.descent);
    switch (annotate->gravity)
    {
      case ForgetGravity:
      case NorthWestGravity:
      default:
      {
        offset.x=(double)geometry.x+(double)i*draw_info->affine.ry*height;
        offset.y=(double)geometry.y+(double)i*draw_info->affine.sy*height;
        break;
      }
      case NorthGravity:
      {
        offset.x=(double)geometry.x+(double)geometry.width/2+(double)i*draw_info->affine.ry*height-
          (double)draw_info->affine.sx*metrics.width/2.0;
        offset.y=(double)geometry.y+(double)i*draw_info->affine.sy*height-draw_info->affine.rx*
          metrics.width/2.0;
        break;
      }
      case NorthEastGravity:
      {
        offset.x=(geometry.width == 0 ? 1.0 : -1.0)*(double)geometry.x+(double)geometry.width+i*
          draw_info->affine.ry*height-(double)draw_info->affine.sx*metrics.width;
        offset.y=(double)geometry.y+(double)i*draw_info->affine.sy*height-(double)draw_info->affine.rx*
          metrics.width;
        break;
      }
      case WestGravity:
      {
        offset.x=(double)geometry.x+(double)i*draw_info->affine.ry*height+(double)draw_info->affine.ry*
          (metrics.ascent+metrics.descent-(double)(number_lines-1)*(double) height)/2.0;
        offset.y=(double) geometry.y+(double)geometry.height/2.0+(double)i*draw_info->affine.sy*height+
          (double)draw_info->affine.sy*((double)metrics.ascent+metrics.descent-(double)(number_lines-1)*
          height)/2.0;
        break;
      }
      case StaticGravity:
      case CenterGravity:
      {
        offset.x=(double)geometry.x+(double)geometry.width/2.0+(double)i*draw_info->affine.ry*height-
          (double)draw_info->affine.sx*metrics.width/2.0+draw_info->affine.ry*
          ((double)metrics.ascent+metrics.descent-(double)(number_lines-1)*height)/2.0;
        offset.y=(double)geometry.y+(double)geometry.height/2.0+(double)i*draw_info->affine.sy*height-
          (double)draw_info->affine.rx*metrics.width/2.0+(double)draw_info->affine.sy*
          ((double)metrics.ascent+metrics.descent-(double)(number_lines-1)*height)/2.0;
        break;
      }
      case EastGravity:
      {
        offset.x=(geometry.width == 0 ? 1.0 : -1.0)*(double)geometry.x+(double)geometry.width+(double)i*
          (double)draw_info->affine.ry*height-(double)draw_info->affine.sx*metrics.width+
          (double)draw_info->affine.ry*((double)metrics.ascent+(double)metrics.descent-(number_lines-1)*
          (double)height)/2.0;
        offset.y=(double)geometry.y+(double)geometry.height/2.0+(double)i*draw_info->affine.sy*height-
            (double)draw_info->affine.rx*metrics.width+(double)draw_info->affine.sy*
          ((double)metrics.ascent+metrics.descent-(number_lines-1)*(double)height)/2.0;
        break;
      }
      case SouthWestGravity:
      {
        offset.x= (double)geometry.x+ (double)i*draw_info->affine.ry*height-draw_info->affine.ry*
          (number_lines-1)*height;
        offset.y= (double)(geometry.height == 0 ? 1 : -1)*geometry.y+geometry.height+i*
          draw_info->affine.sy*height-draw_info->affine.sy*(number_lines-1)*
          height;
        break;
      }
      case SouthGravity:
      {
        offset.x= (double)geometry.x+(double)geometry.width/2.0+(double)i*draw_info->affine.ry*
          height- (double)draw_info->affine.sx*metrics.width/2.0-
            (double)draw_info->affine.ry*(number_lines-1)*height;
        offset.y=(geometry.height == 0 ? 1.0 : -1.0)*geometry.y+geometry.height+(double)i*
          draw_info->affine.sy*height-(double)draw_info->affine.rx*
          metrics.width/2.0-(double)draw_info->affine.sy*(number_lines-1)*height;
        break;
      }
      case SouthEastGravity:
      {
        offset.x=(geometry.width == 0 ? 1.0 : -1.0)*geometry.x+geometry.width+(double)i*
          draw_info->affine.ry*height-(double)draw_info->affine.sx*metrics.width-
            (double)draw_info->affine.ry*(number_lines-1)*height;
        offset.y=(geometry.height == 0 ? 1.0 : -1.0)*geometry.y+(double)geometry.height+i*
          draw_info->affine.sy*height-(double)draw_info->affine.rx*metrics.width-
            (double)draw_info->affine.sy*(number_lines-1)*height;
        break;
      }
    }
    switch (annotate->align)
    {
      case LeftAlign:
      {
        offset.x=geometry.x+i*draw_info->affine.ry*height;
        offset.y=geometry.y+i*draw_info->affine.sy*height;
        break;
      }
      case CenterAlign:
      {
        offset.x=geometry.x+i*draw_info->affine.ry*height-draw_info->affine.sx*
          metrics.width/2;
        offset.y=geometry.y+i*draw_info->affine.sy*height-draw_info->affine.rx*
          metrics.width/2;
        break;
      }
      case RightAlign:
      {
        offset.x=geometry.x+i*draw_info->affine.ry*height-draw_info->affine.sx*
          metrics.width;
        offset.y=geometry.y+i*draw_info->affine.sy*height-draw_info->affine.rx*
          metrics.width;
        break;
      }
      default:
        break;
    }
    if (draw_info->undercolor.opacity != TransparentOpacity)
      {
        /*
          Text box.
        */
        clone_info->fill=draw_info->undercolor;
        clone_info->affine.tx=offset.x-draw_info->affine.ry*(metrics.ascent-
          metrics.max_advance/4);
        clone_info->affine.ty=offset.y-draw_info->affine.sy*metrics.ascent;
        FormatString(primitive,"rectangle 0,0 %g,%ld",metrics.width+
          metrics.max_advance/2.0,height);
        (void) CloneString(&clone_info->primitive,primitive);
        (void) DrawImage(image,clone_info);
      }
    clone_info->affine.tx=offset.x;
    clone_info->affine.ty=offset.y;
    FormatString(primitive,"stroke-width %g line 0,0 %g,0",
      metrics.underline_thickness,metrics.width);
    if (annotate->decorate == OverlineDecoration)
      {
        clone_info->affine.ty-=(draw_info->affine.sy*
          (metrics.ascent+metrics.descent)-metrics.underline_position);
        (void) CloneString(&clone_info->primitive,primitive);
        (void) DrawImage(image,clone_info);
      }
    else
      if (annotate->decorate == UnderlineDecoration)
        {
          clone_info->affine.ty-=metrics.underline_position;
          (void) CloneString(&clone_info->primitive,primitive);
          (void) DrawImage(image,clone_info);
        }
    /*
      Annotate image with text.
    */
    status=RenderType(image,annotate,&offset,&metrics);
    if (status == MagickFail)
      break;
    if (annotate->decorate == LineThroughDecoration)
      {
        clone_info->affine.ty-=(draw_info->affine.sy*height+
          metrics.underline_position)/2.0;
        (void) CloneString(&clone_info->primitive,primitive);
        (void) DrawImage(image,clone_info);
      }
  }
  image->matte=matte;
  /*
    Free resources.
  */
  DestroyDrawInfo(clone_info);
  DestroyDrawInfo(annotate);
  MagickFreeMemory(textlist);
  MagickFreeMemory(text);
  return(status);
}

#if defined(HasTTF)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   E n c o d e S J I S                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EncodeSJIS() converts an ASCII text string to 2-bytes per character code
%  (like UCS-2).  Returns the translated codes and the character count.
%  Characters under 0x7f are just copied, characters over 0x80 are tied with
%  the next character.
%
%  Katsutoshi Shibuya contributed this method.
%
%  The format of the EncodeSJIS function is:
%
%      encoding=EncodeSJIS(const char *text,size_t count)
%
%  A description of each parameter follows:
%
%    o encoding:  EncodeSJIS() returns a pointer to an unsigned short
%      array representing the encoded version of the ASCII string.
%
%    o text: The text.
%
%    o count: return the number of characters generated by the encoding.
%
%
*/

static int GetOneCharacter(const unsigned char *text,size_t *length)
{
  unsigned int
    c;

  if (*length < 1)
    return(-1);
  c=text[0];
  if (!(c & 0x80))
    {
      *length=1;
      return((int) c);
    }
  if (*length < 2)
    {
      *length=0;
      return(-1);
    }
  *length=2;
  c=((int) (text[0]) << 8);
  c|=text[1];
  return((int) c);
}

static magick_code_point_t *EncodeSJIS(const char *text,size_t *count)
{
  int
    c;

  register const char
    *p;

  register magick_code_point_t
    *q;

  size_t
    length;

  magick_code_point_t
    *encoding;

  *count=0;
  if ((text == (char *) NULL) || (*text == '\0'))
    return((magick_code_point_t *) NULL);
  encoding=MagickAllocateArray(magick_code_point_t *,
                               (strlen(text)+MaxTextExtent),
                               sizeof(magick_code_point_t));
  if (encoding == (magick_code_point_t *) NULL)
    MagickFatalError3(ResourceLimitFatalError,MemoryAllocationFailed,
      UnableToConvertText);
  q=encoding;
  for (p=text; *p != '\0'; p+=length)
  {
    length=strlen(p);
    c=GetOneCharacter((const unsigned char *) p,&length);
    if (c < 0)
      {
        q=encoding;
        for (p=text; *p != '\0'; p++)
          *q++=(unsigned char) *p;
        break;
      }
    *q=(magick_code_point_t) c;
    q++;
  }
  *count=q-encoding;
  return(encoding);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   E n c o d e T e x t                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EncodeText() converts an ASCII text string to wide text and returns the
%  translation and the character count.
%
%  The format of the EncodeText function is:
%
%      encoding=EncodeText(const char *text,size_t count)
%
%  A description of each parameter follows:
%
%    o encoding:  EncodeText() returns a pointer to an unsigned short array
%      array representing the encoded version of the ASCII string.
%
%    o text: The text.
%
%    o count: return the number of characters generated by the encoding.
%
%
*/
static magick_code_point_t *EncodeText(const char *text,size_t *count)
{
  register const char
    *p;

  register magick_code_point_t
    *q;

  magick_code_point_t
    *encoding;

  *count=0;
  if ((text == (char *) NULL) || (*text == '\0'))
    return((magick_code_point_t *) NULL);
  encoding=MagickAllocateArray(magick_code_point_t *,
                               (strlen(text)+MaxTextExtent),
                               sizeof(magick_code_point_t));
  if (encoding == (magick_code_point_t *) NULL)
    MagickFatalError3(ResourceLimitFatalError,MemoryAllocationFailed,
      UnableToConvertText);
  q=encoding;
  for (p=text; *p != '\0'; p++)
    *q++=(unsigned char) *p;
  *count=q-encoding;
  return(encoding);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   E n c o d e U n i c o d e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EncodeUnicode() converts an ASCII text string to Unicode and returns the
%  Unicode translation and the character count.  Characters under 0x7f are
%  just copied, characters over 0x80 are tied with the next character.
%
%  The format of the EncodeUnicode function is:
%
%      unicode=EncodeUnicode(const unsigned char *text,size_t count)
%
%  A description of each parameter follows:
%
%    o unicode:  EncodeUnicode() returns a pointer to an unsigned short array
%      array representing the encoded version of the ASCII string.
%
%    o text: The text.
%
%    o count: return the number of characters generated by the encoding.
%
%
*/
static int GetUnicodeCharacter(const unsigned char *text,size_t *length)
{
  unsigned int
    c;

  if (*length < 1)
    return(-1);
  c=text[0];
  if (!(c & 0x80))
    {
      *length=1;
      return((int) c);
    }
  if ((*length < 2) || ((text[1] & 0xc0) != 0x80))
    {
      *length=0;
      return(-1);
    }
  if ((c & 0xe0) != 0xe0)
    {
      *length=2;
      c=(text[0] & 0x1f) << 6;
      c|=text[1] & 0x3f;
      return((int) c);
    }
  if ((*length < 3) || ((text[2] & 0xc0) != 0x80))
    {
      *length=0;
      return(-1);
    }
  if ((c & 0xf0) != 0xf0)
    {
      *length=3;
      c=(text[0] & 0xf) << 12;
      c|=(text[1] & 0x3f) << 6;
      c|=text[2] & 0x3f;
      return((int) c);
    }
  if ((*length < 4) || ((c & 0xf8) != 0xf0) || ((text[3] & 0xc0) != 0x80))
    {
      *length=0;
      return(-1);
    }
  *length=4;
  c=(text[0] & 0x7) << 18;
  c|=(text[1] & 0x3f) << 12;
  c|=(text[2] & 0x3f) << 6;
  c|=text[3] & 0x3f;
  return((int) c);
}

static magick_code_point_t *EncodeUnicode(const char *text,size_t *count)
{
  int
    c;

  register const char
    *p;

  register magick_code_point_t
    *q;

  size_t
    length;

  magick_code_point_t
    *unicode;

  *count=0;
  if ((text == (char *) NULL) || (*text == '\0'))
    return((magick_code_point_t *) NULL);
  unicode=MagickAllocateArray(magick_code_point_t *,
                              (strlen(text)+MaxTextExtent),
                              sizeof(magick_code_point_t));
  if (unicode == (magick_code_point_t *) NULL)
    MagickFatalError3(ResourceLimitFatalError,MemoryAllocationFailed,
      UnableToConvertText);
  q=unicode;
  for (p=text; *p != '\0'; p+=length)
  {
    length=strlen(p);
    c=GetUnicodeCharacter((const unsigned char *) p,&length);
    if (c < 0)
      {
        q=unicode;
        for (p=text; *p != '\0'; p++)
          *q++=(unsigned char) *p;
        break;
      }
    *q=(magick_code_point_t) c;
    q++;
  }
  *count=q-unicode;
  return(unicode);
}

#endif /* HasTTF */
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t T y p e M e t r i c s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetTypeMetrics() returns the following information for the specified font
%  and text:
%
%    o character width
%    o character height
%    o ascent
%    o descent
%    o text width
%    o text height
%    o maximum horizontal advance
%    o underline position
%    o underline thickness
%
%  The format of the GetTypeMetrics method is:
%
%      unsigned int GetTypeMetrics(Image *image,const DrawInfo *draw_info,
%        TypeMetric *metrics)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o metrics: Return the font metrics in this structure.
%
%
*/
MagickExport MagickPassFail GetTypeMetrics(Image *image,const DrawInfo *draw_info,
  TypeMetric *metrics)
{
  DrawInfo
    *clone_info;

  PointInfo
    offset;

  MagickPassFail
    status;

  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->text != (char *) NULL);
  assert(draw_info->signature == MagickSignature);
  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  clone_info->render=False;
  (void) memset(metrics,0,sizeof(TypeMetric));
  offset.x=0.0;
  offset.y=0.0;
  status=RenderType(image,clone_info,&offset,metrics);
  DestroyDrawInfo(clone_info);
  return(status);
}


/*
  Find a single font family name in a comma-separated list; returns a pointer
  to where next search should start (i.e., to the terminating character), or null
  if not found.  Trims leading and trailing white space, and surrounding single
  quotes.
*/
static
char const *FindCommaDelimitedName
(
 char const *  pSearchStart, /*start search here*/
 char const ** ppStart,      /*return pointer to first character in found string*/
 char const ** ppEnd         /*return pointer to just past last character in found string*/
 )
{ /*FindCommaDelimitedName*/

  int c;
  char const * pStart;
  char const * pEnd;
  char const * pNextSearchStart;

  if ( pSearchStart == 0 )
    return(0);

  for ( pStart = pSearchStart; (c = *pStart) && (isspace(c) || (c == ','));
        pStart++ );  /*skip leading spaces and commas*/
  if ( c == '\0' )
    return(0);  /*didn't find anything!*/

  for ( pEnd = pStart + 1; (c = *pEnd) && (c != ','); pEnd++ ); /*find terminating comma*/
  pNextSearchStart = pEnd;

  for ( ; isspace(pEnd[-1]); pEnd-- ); /*trim trailing space; we know there is a non-space character there*/

  /* trim off surrounding single quotes */
  if ((*pStart == '\'') &&  (*pEnd == '\'') && ((pEnd-pStart) >= 3))
    {
      pStart++;
      pEnd--;
    }

  *ppStart = pStart;
  *ppEnd = pEnd;
  return(pNextSearchStart);

} /*FindCommaDelimitedName*/


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e n d e r T y p e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RenderType renders text on the image.  It also returns the bounding
%  box of the text relative to the image.
%
%  The format of the RenderType method is:
%
%      unsigned int RenderType(Image *image,DrawInfo *draw_info,
%        const PointInfo *offset,TypeMetric *metrics)
%
%  A description of each parameter follows:
%
%    o status: Method RenderType returns True if the text is rendered on the
%      image, otherwise False.
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%
*/
static MagickPassFail RenderType(Image *image,const DrawInfo *draw_info,
                                 const PointInfo *offset,TypeMetric *metrics)
{
  const TypeInfo
    *type_info;

  DrawInfo
    *clone_info;

  MagickPassFail
    status;

  char
    OneFontFamilyName[2048]; /*special handling only if font name this long or less*/

  char const *
    pTheFoundFontFamilyName;

  type_info=(const TypeInfo *) NULL;
  if (draw_info->font != (char *) NULL)
    {
      if (*draw_info->font == '@')
        return(RenderFreetype(image,draw_info,(char *) NULL,offset,metrics));
      if (*draw_info->font == '-')
        return(RenderX11(image,draw_info,offset,metrics));
      type_info=GetTypeInfo(draw_info->font,&image->exception);
      if (type_info == (const TypeInfo *) NULL)
        if (IsAccessible(draw_info->font))
          return(RenderFreetype(image,draw_info,(char *) NULL,offset,metrics));
    }

  /* draw_info->family may be a comma-separated list of names ... */
  pTheFoundFontFamilyName = draw_info->family;
  if (type_info == (const TypeInfo *) NULL)
    { /*type_info not yet found*/

      /* stay consistent with previous behavior unless font family contains comma(s) */
      if (draw_info->family == 0 || (strchr(draw_info->family,',') == 0))
        { /*null ptr, or no commas in string; preserve previous behavior*/

          type_info=GetTypeInfoByFamily(draw_info->family,draw_info->style,
                                        draw_info->stretch,draw_info->weight,
                                        &image->exception);

        } /*null ptr, or no commas in string; preserve previous behavior*/
      else
        { /*process as font family list*/

          char const * pNext = draw_info->family, * pStart = 0, * pEnd = 0;
          while ((pNext = FindCommaDelimitedName(pNext,&pStart,&pEnd)) != 0)
            { /*found a name*/

              unsigned int NameLength = pEnd - pStart;
              if  ( NameLength >= sizeof(OneFontFamilyName) )
                continue;
              memcpy(OneFontFamilyName,pStart,NameLength);
              OneFontFamilyName[NameLength] = '\0';
              type_info = GetTypeInfoByFamily(OneFontFamilyName,
                                              draw_info->style,
                                              draw_info->stretch,
                                              draw_info->weight,
                                              &image->exception);
              /*do not allow font substitution*/
              if ( type_info && (LocaleCompare(OneFontFamilyName,
                                               type_info->family) == 0) )
                {
                  pTheFoundFontFamilyName = OneFontFamilyName;
                  break;
                }

            } /*found a name*/

        } /*process as font family list*/

    } /*type_info not yet found*/

  /*
    We may have performed font substitution.  If so (i.e., font family
    name does not match), try again assuming draw_info->family is
    actually a font name.  If we get a font name match, that will
    override the font substitution.
  */
  if ((type_info == (const TypeInfo *) NULL)
      || /*found font family, but ...*/
      (pTheFoundFontFamilyName &&
       (LocaleCompare(pTheFoundFontFamilyName,type_info->family) != 0)))
    {/*either not found, or different font family (probably font substitution)*/

      /* try to match a font name */
      const TypeInfo *type_info2 = 0;
      if (((type_info2 = GetTypeInfo(pTheFoundFontFamilyName,
                                     &image->exception))
           == (const TypeInfo *) NULL)
          && (pTheFoundFontFamilyName != 0)
          && strlen(pTheFoundFontFamilyName) < sizeof(OneFontFamilyName))
        {/*change ' ' to '-' and try again*/

          /*
            Change blanks to hyphens (i.e. make it look like a font
            name vs. font family).  Will only do this for font names
            sizeof(OneFontFamilyName) long or less.
          */
          char FontNameWithHyphens[sizeof(OneFontFamilyName)];
          char *pWithHyphens = FontNameWithHyphens;
          char c;
          const char *pFound;
          for (pFound = pTheFoundFontFamilyName;
               (*pWithHyphens = (((c = *pFound) != ' ') ? c : '-'));
               pFound++, pWithHyphens++);
          type_info2 = GetTypeInfo(FontNameWithHyphens,&image->exception);

        } /*change ' ' to '-' and try again*/

      if  ( type_info2 != (const TypeInfo *) NULL )
        type_info = type_info2;

    } /*either not found, or different font family (probably font substitution)*/

  if (type_info == (const TypeInfo *) NULL)
    return(RenderPostscript(image,draw_info,offset,metrics));
  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  if (type_info->glyphs != (char *) NULL)
    (void) CloneString(&clone_info->font,type_info->glyphs);
  status=RenderFreetype(image,clone_info,type_info->encoding,offset,metrics);
  DestroyDrawInfo(clone_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e n d e r F r e e t y p e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RenderFreetype renders text on the image with a Truetype font.  It
%  also returns the bounding box of the text relative to the image.
%
%  The format of the RenderFreetype method is:
%
%      unsigned int RenderFreetype(Image *image,DrawInfo *draw_info,
%        const char *encoding,const PointInfo *offset,TypeMetric *metrics)
%
%  A description of each parameter follows:
%
%    o status: Method RenderFreetype returns True if the text is rendered on the
%      image, otherwise False.
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o encoding: The font encoding.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%
*/

#if defined(HasTTF)
static int TraceCubicBezier(FT_Vector *p,FT_Vector *q,FT_Vector *to,
  DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MaxTextExtent];

  affine=draw_info->affine;
  FormatString(path,"C%g,%g %g,%g %g,%g",affine.tx+p->x/64.0,
    affine.ty-p->y/64.0,affine.tx+q->x/64.0,affine.ty-q->y/64.0,
    affine.tx+to->x/64.0,affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceLineTo(FT_Vector *to,DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MaxTextExtent];

  affine=draw_info->affine;
  FormatString(path,"L%g,%g",affine.tx+to->x/64.0,affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceMoveTo(FT_Vector *to,DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MaxTextExtent];

  affine=draw_info->affine;
  FormatString(path,"M%g,%g",affine.tx+to->x/64.0,affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceQuadraticBezier(FT_Vector *control,FT_Vector *to,
  DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MaxTextExtent];

  affine=draw_info->affine;
  FormatString(path,"Q%g,%g %g,%g",affine.tx+control->x/64.0,
    affine.ty-control->y/64.0,affine.tx+to->x/64.0,affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static MagickPassFail RenderFreetype(Image *image,const DrawInfo *draw_info,
  const char *encoding,const PointInfo *offset,TypeMetric *metrics)
{
  typedef struct _GlyphInfo
  {
    FT_UInt
      id;

    FT_Vector
      origin;

    FT_Glyph
      image;
  } GlyphInfo;

  double
    opacity;

  DrawInfo
    *clone_info;

  FT_BBox
    bounds;

  FT_BitmapGlyph
    bitmap;

  FT_Encoding
    encoding_type;

  FT_Error
    ft_status;

  FT_Face
    face;

  FT_Library
    library;

  FT_Matrix
    affine;

  FT_Vector
    origin;

  GlyphInfo
    glyph,
    last_glyph;

  Image
    *pattern;

  long
    y;

  PixelPacket
    fill_color;

  PointInfo
    point,
    resolution;

  register long
    i,
    x;

  register PixelPacket
    *q;

  register unsigned char
    *p;

  size_t
    length = 0;

  static FT_Outline_Funcs
    OutlineMethods =
    {
      (FT_Outline_MoveTo_Func) TraceMoveTo,
      (FT_Outline_LineTo_Func) TraceLineTo,
      (FT_Outline_ConicTo_Func) TraceQuadraticBezier,
      (FT_Outline_CubicTo_Func) TraceCubicBezier,
      0, 0
    };

  MagickBool
    active;

  magick_code_point_t
    *text;

  MagickPassFail
    status=MagickPass;

  if (draw_info->font == (char *) NULL)
    ThrowBinaryException(TypeError,FontNotSpecified,image->filename);

  glyph.image=(FT_Glyph) 0;
  last_glyph.image=(FT_Glyph) 0;

  /*
    Initialize Truetype library.
  */
  ft_status=FT_Init_FreeType(&library);
  if (ft_status)
    ThrowBinaryException(TypeError,UnableToInitializeFreetypeLibrary,
                         draw_info->font);
  if (*draw_info->font != '@')
    ft_status=FT_New_Face(library,draw_info->font,0,&face);
  else
    ft_status=FT_New_Face(library,draw_info->font+1,0,&face);
  if (ft_status != 0)
    {
      (void) FT_Done_FreeType(library);
      ThrowBinaryException(TypeError,UnableToReadFont,draw_info->font)
    }
  /*
    Select a charmap
  */
  if (face->num_charmaps != 0)
    /* ft_status= */ (void) FT_Set_Charmap(face,face->charmaps[0]);
  encoding_type=ft_encoding_unicode;
  ft_status=FT_Select_Charmap(face,encoding_type);
  if (ft_status != 0)
    {
      encoding_type=ft_encoding_none;
      /* ft_status= */ (void) FT_Select_Charmap(face,encoding_type);
    }
  if (encoding != (char *) NULL)
    {
      if (LocaleCompare(encoding,"AdobeCustom") == 0)
        encoding_type=ft_encoding_adobe_custom;
      if (LocaleCompare(encoding,"AdobeExpert") == 0)
        encoding_type=ft_encoding_adobe_expert;
      if (LocaleCompare(encoding,"AdobeStandard") == 0)
        encoding_type=ft_encoding_adobe_standard;
      if (LocaleCompare(encoding,"AppleRoman") == 0)
        encoding_type=ft_encoding_apple_roman;
      if (LocaleCompare(encoding,"BIG5") == 0)
        encoding_type=ft_encoding_big5;
      if (LocaleCompare(encoding,"GB2312") == 0)
        encoding_type=ft_encoding_gb2312;
#if defined(ft_encoding_johab)
      if (LocaleCompare(encoding,"Johab") == 0)
        encoding_type=ft_encoding_johab;
#endif
#if defined(ft_encoding_latin_1)
      if (LocaleCompare(encoding,"Latin-1") == 0)
        encoding_type=ft_encoding_latin_1;
#endif
#if defined(ft_encoding_latin_2)
      if (LocaleCompare(encoding,"Latin-2") == 0)
        encoding_type=ft_encoding_latin_2;
#endif
      if (LocaleCompare(encoding,"None") == 0)
        encoding_type=ft_encoding_none;
      if (LocaleCompare(encoding,"SJIScode") == 0)
        encoding_type=ft_encoding_sjis;
      if (LocaleCompare(encoding,"Symbol") == 0)
        encoding_type=ft_encoding_symbol;
      if (LocaleCompare(encoding,"Unicode") == 0)
        encoding_type=ft_encoding_unicode;
      if (LocaleCompare(encoding,"Wansung") == 0)
        encoding_type=ft_encoding_wansung;
      ft_status=FT_Select_Charmap(face,encoding_type);
      if (ft_status != 0)
        ThrowBinaryException(TypeError,UnrecognizedFontEncoding,encoding);
    }
  /*
    Set text size.
  */
  resolution.x=72.0;
  resolution.y=72.0;
  if (draw_info->density != (char *) NULL)
    {
      i=GetMagickDimension(draw_info->density,&resolution.x,&resolution.y,NULL,NULL);
      if (i != 2)
        resolution.y=resolution.x;
    }
  (void) FT_Set_Char_Size(face,(FT_F26Dot6) (64.0*draw_info->pointsize),
    (FT_F26Dot6) (64.0*draw_info->pointsize),(FT_UInt) resolution.x,
    (FT_UInt) resolution.y);
  metrics->pixels_per_em.x=face->size->metrics.x_ppem;
  metrics->pixels_per_em.y=face->size->metrics.y_ppem;
  metrics->ascent=(double) face->size->metrics.ascender/64.0;
  metrics->descent=(double) face->size->metrics.descender/64.0;
  metrics->width=0;
  metrics->height=(double) face->size->metrics.height/64.0;
  metrics->max_advance=(double) face->size->metrics.max_advance/64.0;
  metrics->bounds.x1=0.0;
  metrics->bounds.y1=metrics->descent;
  metrics->bounds.x2=metrics->ascent+metrics->descent;
  metrics->bounds.y2=metrics->ascent+metrics->descent;
  metrics->underline_position=face->underline_position/64.0;
  metrics->underline_thickness=face->underline_thickness/64.0;

  /*
    If the user-provided text string is NULL or empty, then nothing
    more to do.
  */
  if ((draw_info->text == NULL) || (draw_info->text[0] == '\0'))
    {
      (void) FT_Done_Face(face);
      (void) FT_Done_FreeType(library);
      return status;
    }

  /*
    Convert text to 4-byte format (supporting up to 21 code point
    bits) as prescribed by the encoding.
  */
  switch (encoding_type)
  {
    case ft_encoding_sjis:
    {
      text=EncodeSJIS(draw_info->text,&length);
      break;
    }
    case ft_encoding_unicode:
    {
      text=EncodeUnicode(draw_info->text,&length);
      break;
    }
    default:
    {
      if (draw_info->encoding != (char *) NULL)
        {
          if (LocaleCompare(draw_info->encoding,"SJIS") == 0)
            {
              text=EncodeSJIS(draw_info->text,&length);
              break;
            }
          if ((LocaleCompare(draw_info->encoding,"UTF-8") == 0) ||
              (encoding_type != ft_encoding_none))
            {
              text=EncodeUnicode(draw_info->text,&length);
              break;
            }
        }
      text=EncodeText(draw_info->text,&length);
      break;
    }
  }
  if (text == (magick_code_point_t *) NULL)
    {
      (void) FT_Done_Face(face);
      (void) FT_Done_FreeType(library);
      (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),
                            "Text encoding failed: encoding_type=%ld "
                            "draw_info->encoding=\"%s\" draw_info->text=\"%s\" length=%ld",
                            (long) encoding_type,
                            (draw_info->encoding ? draw_info->encoding : "(null)"),
                            (draw_info->text ? draw_info->text : "(null)"),
                            (long) length);
      ThrowBinaryException(ResourceLimitError,MemoryAllocationFailed,
        draw_info->font)
    }
  /*
    Compute bounding box.
  */
  (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),
    "Font %.1024s; font-encoding %.1024s; text-encoding %.1024s; pointsize %g",
    draw_info->font != (char *) NULL ? draw_info->font : "none",
    encoding != (char *) NULL ? encoding : "none",
    draw_info->encoding != (char *) NULL ? draw_info->encoding : "none",
    draw_info->pointsize);
  glyph.id=0;
  last_glyph.id=0;
  origin.x=0;
  origin.y=0;
  affine.xx=(FT_Fixed) (65536L*draw_info->affine.sx+0.5);
  affine.yx=(FT_Fixed) (-65536L*draw_info->affine.rx+0.5);
  affine.xy=(FT_Fixed) (-65536L*draw_info->affine.ry+0.5);
  affine.yy=(FT_Fixed) (65536L*draw_info->affine.sy+0.5);
  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  (void) QueryColorDatabase("#000000ff",&clone_info->fill,&image->exception);
  (void) CloneString(&clone_info->primitive,"path '");
  pattern=draw_info->fill_pattern;
  for (i=0; i < (long) length; i++)
  {
    glyph.id=FT_Get_Char_Index(face,text[i]);
    if ((glyph.id != 0) && (last_glyph.id != 0) && FT_HAS_KERNING(face))
      {
        FT_Vector
          kerning;

        (void) FT_Get_Kerning(face,last_glyph.id,glyph.id,ft_kerning_default,
          &kerning);
        origin.x+=kerning.x;
      }
    glyph.origin=origin;
    glyph.image=0;
    ft_status=FT_Load_Glyph(face,glyph.id,FT_LOAD_DEFAULT);
    if (ft_status != False) /* 0 means success */
      continue;
    ft_status=FT_Get_Glyph(face->glyph,&glyph.image);
    if (ft_status != False) /* 0 means success */
      continue;
#if 0
    /*
      Obtain glyph's control box. Usually faster than computing the
      exact bounding box but may be slightly larger in some
      situations.
    */
    (void) FT_Glyph_Get_CBox(glyph.image,FT_GLYPH_BBOX_SUBPIXELS,&bounds);
#else
    /*
      Compute exact bounding box for scaled outline. If necessary, the
      outline Bezier arcs are walked over to extract their extrema.
    */
    (void) FT_Outline_Get_BBox(&((FT_OutlineGlyph) glyph.image)->outline,&bounds);
#endif
    if ((i == 0) || (bounds.xMin < metrics->bounds.x1))
      metrics->bounds.x1=bounds.xMin;
    if ((i == 0) || (bounds.yMin < metrics->bounds.y1))
      metrics->bounds.y1=bounds.yMin;
    if ((i == 0) || (bounds.xMax > metrics->bounds.x2))
      metrics->bounds.x2=bounds.xMax;
    if ((i == 0) || (bounds.yMax > metrics->bounds.y2))
      metrics->bounds.y2=bounds.yMax;
    if (draw_info->render)
      if ((draw_info->stroke.opacity != TransparentOpacity) ||
          (draw_info->stroke_pattern != (Image *) NULL))
        {
          /*
            Trace the glyph.
          */
          clone_info->affine.tx=glyph.origin.x/64.0;
          clone_info->affine.ty=glyph.origin.y/64.0;
          (void) FT_Outline_Decompose(&((FT_OutlineGlyph) glyph.image)->outline,
            &OutlineMethods,clone_info);
        }
    FT_Vector_Transform(&glyph.origin,&affine);
    (void) FT_Glyph_Transform(glyph.image,&affine,&glyph.origin);
    if (draw_info->render)
      {
        status &= ModifyCache(image,&image->exception);
        if ((draw_info->fill.opacity != TransparentOpacity) ||
            (pattern != (Image *) NULL))
          {
            /*
              Rasterize the glyph.
            */
            ft_status=FT_Glyph_To_Bitmap(&glyph.image,ft_render_mode_normal,
              (FT_Vector *) NULL,True);
            if (ft_status != False)
              continue;
            bitmap=(FT_BitmapGlyph) glyph.image;
            image->storage_class=DirectClass;
            if (bitmap->bitmap.pixel_mode == ft_pixel_mode_mono)
              {
                point.x=offset->x+(origin.x >> 6);
              }
            else
              {
                point.x=offset->x+bitmap->left;
              }
            point.y=offset->y-bitmap->top;
            p=bitmap->bitmap.buffer;
            /* FIXME: OpenMP */
            for (y=0; y < (long) bitmap->bitmap.rows; y++)
            {
              int pc = y * bitmap->bitmap.pitch;
              int pcr = pc;
              if ((ceil(point.y+y-0.5) < 0) ||
                  (ceil(point.y+y-0.5) >= image->rows))
                {
                  continue;
                }
              /*
                Try to get whole span.  May fail.
              */
              q=GetImagePixels(image,(long) ceil(point.x-0.5),
                (long) ceil(point.y+y-0.5),bitmap->bitmap.width,1);
              active=q != (PixelPacket *) NULL;
              for (x=0; x < (long) bitmap->bitmap.width; x++, pc++)
                {
                  if (((long) ceil(point.x+x-0.5) < 0) ||
                      ((unsigned long) ceil(point.x+x-0.5) >= image->columns))
                    {
                      if (active)
                        q++;
                      continue;
                    }
                  /* 8-bit gray-level pixmap */
                  if (bitmap->bitmap.pixel_mode == ft_pixel_mode_grays)
                    {
                      if (draw_info->text_antialias)
                        opacity=ScaleCharToQuantum((double) p[pc]);
                      else
                        opacity=(p[pc] < 127 ? OpaqueOpacity : TransparentOpacity);
                    }
                  /* 1-bit monochrome bitmap */
                  else if (bitmap->bitmap.pixel_mode == ft_pixel_mode_mono)
                    {
                      opacity=((p[(x >> 3) + pcr] & (1 << (~x & 0x07))) ?
                               TransparentOpacity : OpaqueOpacity);
                    }
                  else
                    {
                      continue; /* ignore it? */
                    }
                  fill_color=draw_info->fill;
                  if (pattern != (Image *) NULL)
                    {
                      if (AcquireOnePixelByReference
                          (pattern,&fill_color,
                           (long) (point.x+x-pattern->tile_info.x) % pattern->columns,
                           (long) (point.y+y-pattern->tile_info.y) % pattern->rows,
                           &image->exception) == MagickFail)
                        {
                          status=MagickFail;
                        }
                    }
                  /*
                    If not full span, then get one pixel.
                  */
                  if (!active)
                    q=GetImagePixels(image,(long) ceil(point.x+x-0.5),
                                     (long) ceil(point.y+y-0.5),1,1);
                  if (q == (PixelPacket *) NULL)
                    {
                      continue;
                    }
                  /*
                    At this point, opacity is 0==transparent to MaxRGB==opaque, and represents an
                    anti-aliasing edge blending value.  The computation below integrates in the
                    fill color opacity, and converts the result to 0=opaque to MaxRGB=transparent.
                  */
                  opacity=MaxRGB-(opacity*(MaxRGB-fill_color.opacity)+/*round*/(MaxRGB>>1))/MaxRGB;
                  AlphaCompositePixel(q,&fill_color,opacity,q,
                                      image->matte ? q->opacity : OpaqueOpacity);
                  if (!active)
                    {
                      /*
                        Sync the one pixel
                      */
                      if (SyncImagePixels(image) != MagickPass)
                        status=MagickFail;
                    }
                  else
                    {
                      q++;
                    }
                  if (status == MagickFail)
                    break;
                }
              /*
                Sync the full span
              */
              if (active)
                if (SyncImagePixels(image) != MagickPass)
                  status=MagickFail;
              if (status == MagickFail)
                break;
            }
          }
      }
    origin.x+=face->glyph->advance.x;
    if (origin.x > metrics->width)
      metrics->width=origin.x;
    if (last_glyph.image != 0)
      {
        FT_Done_Glyph(last_glyph.image);
        last_glyph.image=0;
      }
    last_glyph=glyph;
  }
  metrics->width/=64.0;
  metrics->bounds.x1/=64.0;
  metrics->bounds.y1/=64.0;
  metrics->bounds.x2/=64.0;
  metrics->bounds.y2/=64.0;
  if ((status != MagickFail)&& (draw_info->render))
    if ((draw_info->stroke.opacity != TransparentOpacity) ||
        (draw_info->stroke_pattern != (Image *) NULL))
      {
        /*
          Draw text stroke.
        */
        clone_info->affine.tx=offset->x;
        clone_info->affine.ty=offset->y;
        (void) ConcatenateString(&clone_info->primitive,"'");
        (void) DrawImage(image,clone_info);
      }
  if (glyph.image != 0)
    {
      FT_Done_Glyph(glyph.image);
      glyph.image=0;
    }
  /*
    Free resources.
  */
  MagickFreeMemory(text);
  DestroyDrawInfo(clone_info);
  (void) FT_Done_Face(face);
  (void) FT_Done_FreeType(library);
  return(status);
}
#else
static unsigned int RenderFreetype(Image *image,const DrawInfo *draw_info,
  const char *encoding,const PointInfo *offset,
  TypeMetric *metrics)
{
  ThrowBinaryException(MissingDelegateError,FreeTypeLibraryIsNotAvailable,
                       draw_info->font);
  ARG_NOT_USED(encoding);
  ARG_NOT_USED(offset);
  ARG_NOT_USED(metrics);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e n d e r P o s t s c r i p t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RenderPostscript renders text on the image with a Postscript font.
%  It also returns the bounding box of the text relative to the image.
%
%  The format of the RenderPostscript method is:
%
%      unsigned int RenderPostscript(Image *image,DrawInfo *draw_info,
%        const PointInfo *offset,TypeMetric *metrics)
%
%  A description of each parameter follows:
%
%    o status: Method RenderPostscript returns True if the text is rendered on
%      the image, otherwise False.
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%
*/

static char *EscapeParenthesis(const char *source)
{
  char
    *destination;

  register char
    *q;

  register const char
    *p;

  size_t
    length;

  assert(source != (const char *) NULL);

  /*
    Use dry-run method to compute required string length.
  */
  length=0;
  for (p=source; *p; p++)
    {
      if ((*p == '(') || (*p == ')'))
        length++;
      length++;
    }
  destination=MagickAllocateMemory(char *,length+1);
  if (destination == (char *) NULL)
    MagickFatalError3(ResourceLimitFatalError,MemoryAllocationFailed,
      UnableToEscapeString);
  *destination='\0';
  q=destination;
  for (p=source; *p; p++)
    {
      if ((*p == '(') || (*p == ')'))
        *q++= '\\';
      *q++=(*p);
    }
  *q=0;
  return(destination);
}

static MagickPassFail RenderPostscript(Image *image,const DrawInfo *draw_info,
  const PointInfo *offset,TypeMetric *metrics)
{
  char
    filename[MaxTextExtent],
    geometry[MaxTextExtent],
    *text;

  FILE
    *file;

  Image
    *annotate_image,
    *pattern;

  ImageInfo
    *clone_info;

  long
    y;

  PointInfo
    extent,
    point,
    resolution;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned int
    identity;

  /*
    Render label with a Postscript font.
  */
  (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),
    "Font %.1024s; pointsize %g",draw_info->font != (char *) NULL ?
    draw_info->font : "none",draw_info->pointsize);
  file=AcquireTemporaryFileStream(filename,BinaryFileIOMode);
  if (file == (FILE *) NULL)
    ThrowBinaryException(FileOpenError,UnableToCreateTemporaryFile,filename);
  (void) fprintf(file,"%%!PS-Adobe-3.0\n");
  (void) fprintf(file,"/ReencodeType\n");
  (void) fprintf(file,"{\n");
  (void) fprintf(file,"  findfont dup length\n");
  (void) fprintf(file,
    "  dict begin { 1 index /FID ne {def} {pop pop} ifelse } forall\n");
  (void) fprintf(file,
    "  /Encoding ISOLatin1Encoding def currentdict end definefont pop\n");
  (void) fprintf(file,"} bind def\n");
  /*
    Sample to compute bounding box.
  */
  identity=(draw_info->affine.sx == draw_info->affine.sy) &&
    (draw_info->affine.rx == 0.0) && (draw_info->affine.ry == 0.0);
  extent.x=0.0;
  extent.y=0.0;
  for (i=0; i <= (long) (strlen(draw_info->text)+2); i++)
  {
    point.x=fabs(draw_info->affine.sx*i*draw_info->pointsize+
      draw_info->affine.ry*2.0*draw_info->pointsize);
    point.y=fabs(draw_info->affine.rx*i*draw_info->pointsize+
      draw_info->affine.sy*2.0*draw_info->pointsize);
    if (point.x > extent.x)
      extent.x=point.x;
    if (point.y > extent.y)
      extent.y=point.y;
  }
  (void) fprintf(file,"%g %g moveto\n",identity ? 0.0 : extent.x/2.0,
    extent.y/2.0);
  (void) fprintf(file,"%g %g scale\n",draw_info->pointsize,
    draw_info->pointsize);
  if ((draw_info->font == (char *) NULL) || (*draw_info->font == '\0'))
    (void) fprintf(file,
      "/Times-Roman-ISO dup /Times-Roman ReencodeType findfont setfont\n");
  else
    (void) fprintf(file,
      "/%.1024s-ISO dup /%.1024s ReencodeType findfont setfont\n",
      draw_info->font,draw_info->font);
  (void) fprintf(file,"[%g %g %g %g 0 0] concat\n",draw_info->affine.sx,
    -draw_info->affine.rx,-draw_info->affine.ry,draw_info->affine.sy);
  text=EscapeParenthesis(draw_info->text);
  if (!identity)
    (void) fprintf(file,"(%.1024s) stringwidth pop -0.5 mul -0.5 rmoveto\n",
      text);
  (void) fprintf(file,"(%.1024s) show\n",text);
  MagickFreeMemory(text);
  (void) fprintf(file,"showpage\n");
  (void) fclose(file);
  FormatString(geometry,"%ldx%ld+0+0!",(long) ceil(extent.x-0.5),
    (long) ceil(extent.y-0.5));
  clone_info=CloneImageInfo((ImageInfo *) NULL);
  (void) FormatString(clone_info->filename,"ps:%.1024s",filename);
  (void) CloneString(&clone_info->page,geometry);
  if (draw_info->density != (char *) NULL)
    (void) CloneString(&clone_info->density,draw_info->density);
  clone_info->antialias=draw_info->text_antialias;
  annotate_image=ReadImage(clone_info,&image->exception);
  if (image->exception.severity != UndefinedException)
    MagickError2(image->exception.severity,image->exception.reason,
      image->exception.description);
  DestroyImageInfo(clone_info);
  (void) LiberateTemporaryFile(filename);
  if (annotate_image == (Image *) NULL)
    return(False);
  resolution.x=72.0;
  resolution.y=72.0;
  if (draw_info->density != (char *) NULL)
    {
      int
        count;

      count=GetMagickDimension(draw_info->density,&resolution.x,&resolution.y,NULL,NULL);
      if (count != 2)
        resolution.y=resolution.x;
    }
  if (!identity)
    TransformImage(&annotate_image,"0x0",(char *) NULL);
  else
    {
      RectangleInfo
        crop_info;

      crop_info=GetImageBoundingBox(annotate_image,&annotate_image->exception);
      crop_info.height=(unsigned long) ceil((resolution.y/72.0)*
        ExpandAffine(&draw_info->affine)*draw_info->pointsize-0.5);
      crop_info.y=(long) ceil((resolution.y/72.0)*extent.y/8.0-0.5);
      (void) FormatString(geometry,"%lux%lu%+ld%+ld",crop_info.width,
        crop_info.height,crop_info.x,crop_info.y);
      TransformImage(&annotate_image,geometry,(char *) NULL);
    }
  metrics->pixels_per_em.x=(resolution.y/72.0)*
    ExpandAffine(&draw_info->affine)*draw_info->pointsize;
  metrics->pixels_per_em.y=metrics->pixels_per_em.x;
  metrics->ascent=metrics->pixels_per_em.x;
  metrics->descent=metrics->pixels_per_em.y/-5.0;
  metrics->width=annotate_image->columns/ExpandAffine(&draw_info->affine);
  metrics->height=1.152*metrics->pixels_per_em.x;
  metrics->max_advance=metrics->pixels_per_em.x;
  metrics->bounds.x1=0.0;
  metrics->bounds.y1=metrics->descent;
  metrics->bounds.x2=metrics->ascent+metrics->descent;
  metrics->bounds.y2=metrics->ascent+metrics->descent;
  metrics->underline_position=(-2.0);
  metrics->underline_thickness=1.0;
  if (!draw_info->render)
    {
      DestroyImage(annotate_image);
      return(True);
    }
  if (draw_info->fill.opacity != TransparentOpacity)
    {
      PixelPacket
        fill_color;

      /*
        Render fill color.
      */
      (void) SetImageType(annotate_image,TrueColorMatteType);
      fill_color=draw_info->fill;
      pattern=draw_info->fill_pattern;
      for (y=0; y < (long) annotate_image->rows; y++)
      {
        q=GetImagePixels(annotate_image,0,y,annotate_image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) annotate_image->columns; x++)
        {
          if (pattern != (Image *) NULL)
            (void) AcquireOnePixelByReference(pattern,&fill_color,
              (long) (x-pattern->tile_info.x) % pattern->columns,
              (long) (y-pattern->tile_info.y) % pattern->rows,
              &image->exception);
          q->opacity=(Quantum) (MaxRGB-(((MaxRGB-(double)
            PixelIntensityToQuantum(q))*(MaxRGB-fill_color.opacity))/
            MaxRGB)+0.5);
          q->red=fill_color.red;
          q->green=fill_color.green;
          q->blue=fill_color.blue;
          q++;
        }
        if (!SyncImagePixels(annotate_image))
          break;
      }
      (void) CompositeImage(image,OverCompositeOp,annotate_image,(long)
        ceil(offset->x-0.5),(long) ceil(offset->y-(metrics->ascent+
        metrics->descent)-0.5));
    }
  DestroyImage(annotate_image);
  return(MagickPass);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e n d e r X 1 1                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RenderX11 renders text on the image with an X11 font.  It also
%  returns the bounding box of the text relative to the image.
%
%  The format of the RenderX11 method is:
%
%      unsigned int RenderX11(Image *image,DrawInfo *draw_info,
%        const PointInfo *offset,TypeMetric *metrics)
%
%  A description of each parameter follows:
%
%    o status: Method RenderX11 returns True if the text is rendered on the
%      image, otherwise False.
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%
*/
#if defined(HasX11)
static MagickPassFail RenderX11(Image *image,const DrawInfo *draw_info,
  const PointInfo *offset,TypeMetric *metrics)
{
  static DrawInfo
    cache_info;

  static Display
    *display = (Display *) NULL;

  static MagickXAnnotateInfo
    annotate_info;

  static XFontStruct
    *font_info;

  static MagickXPixelInfo
    pixel;

  static MagickXResourceInfo
    resource_info;

  static XrmDatabase
    resource_database;

  static XStandardColormap
    *map_info;

  static XVisualInfo
    *visual_info;

  MagickPassFail
    status;

  unsigned long
    height,
    width;

  if (display == (Display *) NULL)
    {
      const char
        *client_name;

      /*
        Open X server connection.
      */
      display=XOpenDisplay(draw_info->server_name);
      if (display == (Display *) NULL)
        ThrowBinaryException(XServerError,UnableToOpenXServer,
          draw_info->server_name);
      /*
        Get user defaults from X resource database.
      */
      (void) XSetErrorHandler(MagickXError);
      client_name=GetClientName();
      resource_database=MagickXGetResourceDatabase(display,client_name);
      MagickXGetResourceInfo(resource_database,client_name,&resource_info);
      resource_info.close_server=False;
      resource_info.colormap=PrivateColormap;
      resource_info.font=AllocateString(draw_info->font);
      resource_info.background_color=AllocateString("#ffffffffffff");
      resource_info.foreground_color=AllocateString("#000000000000");
      map_info=XAllocStandardColormap();
      if (map_info == (XStandardColormap *) NULL)
        ThrowBinaryException3(ResourceLimitError,MemoryAllocationFailed,
          UnableToAllocateColormap);
      /*
        Initialize visual info.
      */
      visual_info=MagickXBestVisualInfo(display,map_info,&resource_info);
      if (visual_info == (XVisualInfo *) NULL)
        ThrowBinaryException(XServerError,UnableToGetVisual,
          draw_info->server_name);
      map_info->colormap=(Colormap) NULL;
      pixel.pixels=(unsigned long *) NULL;
      /*
        Initialize Standard Colormap info.
      */
      MagickXGetMapInfo(visual_info,XDefaultColormap(display,visual_info->screen),
        map_info);
      MagickXGetPixelPacket(display,visual_info,map_info,&resource_info,
        (Image *) NULL,&pixel);
      pixel.annotate_context=XDefaultGC(display,visual_info->screen);
      /*
        Initialize font info.
      */
      font_info=MagickXBestFont(display,&resource_info,False);
      if (font_info == (XFontStruct *) NULL)
        ThrowBinaryException(XServerError,UnableToLoadFont,draw_info->font);
      cache_info=(*draw_info);
    }
  /*
    Initialize annotate info.
  */
  MagickXGetAnnotateInfo(&annotate_info);
  annotate_info.stencil=ForegroundStencil;
  if (cache_info.font != draw_info->font)
    {
      /*
        Type name has changed.
      */
      (void) XFreeFont(display,font_info);
      (void) CloneString(&resource_info.font,draw_info->font);
      font_info=MagickXBestFont(display,&resource_info,False);
      if (font_info == (XFontStruct *) NULL)
        ThrowBinaryException(XServerError,UnableToLoadFont,draw_info->font);
    }
  (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),
    "Font %.1024s; pointsize %g",draw_info->font != (char *) NULL ?
    draw_info->font : "none",draw_info->pointsize);
  cache_info=(*draw_info);
  annotate_info.font_info=font_info;
  annotate_info.text=(char *) draw_info->text;
  annotate_info.width=XTextWidth(font_info,draw_info->text,
    (int) strlen(draw_info->text));
  annotate_info.height=font_info->ascent+font_info->descent;
  metrics->pixels_per_em.x=font_info->max_bounds.width;
  metrics->pixels_per_em.y=font_info->max_bounds.width;
  metrics->ascent=font_info->ascent;
  metrics->descent=(-font_info->descent);
  metrics->width=annotate_info.width/ExpandAffine(&draw_info->affine);
  metrics->height=metrics->pixels_per_em.x+4;
  metrics->max_advance=font_info->max_bounds.width;
  metrics->bounds.x1=0.0;
  metrics->bounds.y1=metrics->descent;
  metrics->bounds.x2=metrics->ascent+metrics->descent;
  metrics->bounds.y2=metrics->ascent+metrics->descent;
  metrics->underline_position=(-2.0);
  metrics->underline_thickness=1.0;
  if (!draw_info->render)
    return(MagickPass);
  if (draw_info->fill.opacity == TransparentOpacity)
    return(MagickPass);
  /*
    Render fill color.
  */
  width=annotate_info.width;
  height=annotate_info.height;
  if ((draw_info->affine.rx != 0.0) || (draw_info->affine.ry != 0.0))
    {
      if (((draw_info->affine.sx-draw_info->affine.sy) == 0.0) &&
          ((draw_info->affine.rx+draw_info->affine.ry) == 0.0))
        annotate_info.degrees=(180.0/MagickPI)*
          atan2(draw_info->affine.rx,draw_info->affine.sx);
    }
  FormatString(annotate_info.geometry,"%lux%lu+%ld+%ld",width,height,
    (long) ceil(offset->x-0.5),
    (long) ceil(offset->y-metrics->ascent-metrics->descent-0.5));
  pixel.pen_color.red=ScaleQuantumToShort(draw_info->fill.red);
  pixel.pen_color.green=ScaleQuantumToShort(draw_info->fill.green);
  pixel.pen_color.blue=ScaleQuantumToShort(draw_info->fill.blue);
  status=MagickXAnnotateImage(display,&pixel,&annotate_info,image);
  if (status == 0)
    ThrowBinaryException3(ResourceLimitError,MemoryAllocationFailed,
      UnableToAnnotateImage);
  return(MagickPass);
}
#else
static MagickPassFail RenderX11(Image *image,const DrawInfo *draw_info,
  const PointInfo *offset,TypeMetric *metrics)
{
  ARG_NOT_USED(offset);
  ARG_NOT_USED(metrics);
  ThrowBinaryException(MissingDelegateError,XWindowLibraryIsNotAvailable,
    draw_info->font);
}
#endif
