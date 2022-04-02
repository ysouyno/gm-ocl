/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  SSSSS   TTTTT  RRRR   IIIII  N   N   GGGG                  %
%                  SS        T    R   R    I    NN  N  G                      %
%                   SSS      T    RRRR     I    N N N  G GGG                  %
%                     SS     T    R R      I    N  NN  G   G                  %
%                  SSSSS     T    R  R   IIIII  N   N   GGGG                  %
%                                                                             %
%                                                                             %
%                        MagickCore String Methods                            %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                               August 2003                                   %
%                                                                             %
%                                                                             %
%  Copyright @ 2003 ImageMagick Studio LLC, a non-profit organization         %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the license.  You may  %
%  obtain a copy of the license at                                            %
%                                                                             %
%    https://imagemagick.org/script/license.php                               %
%                                                                             %
%  unless required by applicable law or agreed to in writing, software        %
%  distributed under the license is distributed on an "as is" basis,          %
%  without warranties or conditions of any kind, either express or implied.   %
%  See the license for the specific language governing permissions and        %
%  limitations under the license.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/


/*
  Modifications copyright (C) 2022 ysouyno
*/

/*
  Include declarations.
*/
// #include "MagickCore/studio.h"
// #include "MagickCore/blob.h"
// #include "MagickCore/blob-private.h"
// #include "MagickCore/exception.h"
// #include "MagickCore/exception-private.h"
// #include "MagickCore/image-private.h"
// #include "MagickCore/list.h"
// #include "MagickCore/locale_.h"
// #include "MagickCore/log.h"
// #include "MagickCore/memory_.h"
// #include "MagickCore/memory-private.h"
// #include "MagickCore/nt-base-private.h"
// #include "MagickCore/property.h"
// #include "MagickCore/resource_.h"
// #include "MagickCore/signature-private.h"
// #include "MagickCore/string_.h"
// #include "MagickCore/string-private.h"
// #include "MagickCore/utility-private.h"
#include "magick/im_common.h"
#include "magick/blob.h"

/*
  Define declarations.
*/
#define CharsPerLine  0x14


#if defined(HAVE_OPENCL)

// TODO(ocl): Copy from GM's magick_compat.c
void *RelinquishMagickMemory(void *memory)
{
  if (memory == (void *) NULL)
    return((void *) NULL);
  free(memory);
  return((void *) NULL);
}

static StringInfo *AcquireStringInfoContainer()
{
  StringInfo
    *string_info;

  string_info=(StringInfo *) AcquireCriticalMemory(sizeof(*string_info));
  (void) memset(string_info,0,sizeof(*string_info));
  string_info->signature=MagickSignature;
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n f i g u r e F i l e T o S t r i n g I n f o                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConfigureFileToStringInfo() returns the contents of a configure file as a
%  string.
%
%  The format of the ConfigureFileToStringInfo method is:
%
%      StringInfo *ConfigureFileToStringInfo(const char *filename)
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: the filename.
%
*/
MagickExport StringInfo *ConfigureFileToStringInfo(const char *filename)
{
  char
    *string;

  int
    file;

  MagickOffsetType
    offset;

  size_t
    length;

  StringInfo
    *string_info;

  void
    *map;

  assert(filename != (const char *) NULL);
  file=open/*_utf8*/(filename,O_RDONLY | O_BINARY,0);
  if (file == -1)
    return((StringInfo *) NULL);
  offset=(MagickOffsetType) lseek(file,0,SEEK_END);
  if ((offset < 0) || (offset != (MagickOffsetType) ((ssize_t) offset)))
    {
      file=close(file)-1;
      return((StringInfo *) NULL);
    }
  length=(size_t) offset;
  string=(char *) NULL;
  if (~length >= (MagickPathExtent-1))
    string=(char *) AcquireQuantumMemory(length+MagickPathExtent,
      sizeof(*string));
  if (string == (char *) NULL)
    {
      file=close(file)-1;
      return((StringInfo *) NULL);
    }
  map=MapBlob(file,ReadMode,0,length);
  if (map != (void *) NULL)
    {
      (void) memcpy(string,map,length);
      (void) UnmapBlob(map,length);
    }
  else
    {
      size_t
        i;

      ssize_t
        count;

      (void) lseek(file,0,SEEK_SET);
      for (i=0; i < length; i+=count)
      {
        count=read(file,string+i,(size_t) MagickMin(length-i,(size_t)
          MAGICK_SSIZE_MAX));
        if (count <= 0)
          {
            count=0;
            if (errno != EINTR)
              break;
          }
      }
      if (i < length)
        {
          file=close(file)-1;
          string=DestroyString(string);
          return((StringInfo *) NULL);
        }
    }
  string[length]='\0';
  file=close(file)-1;
  string_info=AcquireStringInfoContainer();
  string_info->path=ConstantString(filename);
  string_info->length=length;
  string_info->datum=(unsigned char *) string;
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n s t a n t S t r i n g                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConstantString() allocates exactly the needed memory for a string and
%  copies the source string to that memory location.  A NULL string pointer
%  will allocate an empty string containing just the NUL character.
%
%  When finished the string should be freed using DestoryString()
%
%  The format of the ConstantString method is:
%
%      char *ConstantString(const char *source)
%
%  A description of each parameter follows:
%
%    o source: A character string.
%
*/
MagickExport char *ConstantString(const char *source)
{
  char
    *destination;

  size_t
    length;

  length=0;
  if (source != (char *) NULL)
    length+=strlen(source);
  destination=(char *) NULL;
  if (~length >= 1UL)
    destination=(char *) AcquireQuantumMemory(length+1UL,sizeof(*destination));
  // if (destination == (char *) NULL)
  //   ThrowFatalException(ResourceLimitFatalError,"UnableToAcquireString");
  if (source != (char *) NULL)
    (void) memcpy(destination,source,length*sizeof(*destination));
  destination[length]='\0';
  return(destination);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o p y M a g i c k S t r i n g                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CopyMagickString() copies the source string to the destination string, with
%  out exceeding the given pre-declared length.
%
%  The destination buffer is always null-terminated even if the string must be
%  truncated.  The return value is the length of the string.
%
%  The format of the CopyMagickString method is:
%
%      size_t CopyMagickString(const char *magick_restrict destination,
%        char *magick_restrict source,const size_t length)
%
%  A description of each parameter follows:
%
%    o destination: the destination string.
%
%    o source: the source string.
%
%    o length: the length of the destination string.
%
*/
MagickExport size_t CopyMagickString(char *magick_restrict destination,
  const char *magick_restrict source,const size_t length)
{
  char
    *magick_restrict q;

  const char
    *magick_restrict p;

  size_t
    n;

  p=source;
  q=destination;
  for (n=length; n > 4; n-=4)
  {
    if (((*q++)=(*p++)) == '\0')
      return((size_t) (p-source-1));
    if (((*q++)=(*p++)) == '\0')
      return((size_t) (p-source-1));
    if (((*q++)=(*p++)) == '\0')
      return((size_t) (p-source-1));
    if (((*q++)=(*p++)) == '\0')
      return((size_t) (p-source-1));
  }
  if (length != 0)
    {
      while (--n != 0)
        if (((*q++)=(*p++)) == '\0')
          return((size_t) (p-source-1));
      *q='\0';
    }
  return((size_t) (p-source));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y S t r i n g                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyString() destroys memory associated with a string.
%
%  The format of the DestroyString method is:
%
%      char *DestroyString(char *string)
%
%  A description of each parameter follows:
%
%    o string: the string.
%
*/
MagickExport char *DestroyString(char *string)
{
  return((char *) RelinquishMagickMemory(string));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y S t r i n g I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyStringInfo() destroys memory associated with the StringInfo structure.
%
%  The format of the DestroyStringInfo method is:
%
%      StringInfo *DestroyStringInfo(StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
*/
MagickExport StringInfo *DestroyStringInfo(StringInfo *string_info)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickSignature);
  if (string_info->datum != (unsigned char *) NULL)
    string_info->datum=(unsigned char *) RelinquishMagickMemory(
      string_info->datum);
  if (string_info->name != (char *) NULL)
    string_info->name=DestroyString(string_info->name);
  if (string_info->path != (char *) NULL)
    string_info->path=DestroyString(string_info->path);
  string_info->signature=(~MagickSignature);
  string_info=(StringInfo *) RelinquishMagickMemory(string_info);
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t E n v i r o n m e n t V a l u e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetEnvironmentValue() returns the environment string that matches the
%  specified name.
%
%  The format of the GetEnvironmentValue method is:
%
%      char *GetEnvironmentValue(const char *name)
%
%  A description of each parameter follows:
%
%    o name: the environment name.
%
*/
MagickExport char *GetEnvironmentValue(const char *name)
{
  const char
    *environment;

  environment=getenv(name);
  if (environment == (const char *) NULL)
    return((char *) NULL);
  return(ConstantString(environment));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t S t r i n g I n f o D a t u m                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetStringInfoDatum() returns the datum associated with the string.
%
%  The format of the GetStringInfoDatum method is:
%
%      unsigned char *GetStringInfoDatum(const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string info.
%
*/
MagickExport unsigned char *GetStringInfoDatum(const StringInfo *string_info)
{
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickSignature);
  return(string_info->datum);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s S t r i n g T r u e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsStringTrue() returns MagickTrue if the value is "true", "on", "yes" or
%  "1". Any other string or undefined returns MagickFalse.
%
%  Typically this is used to look at strings (options or artifacts) which
%  has a default value of "false", when not defined.
%
%  The format of the IsStringTrue method is:
%
%      MagickBooleanType IsStringTrue(const char *value)
%
%  A description of each parameter follows:
%
%    o value: Specifies a pointer to a character array.
%
*/
MagickExport MagickBooleanType IsStringTrue(const char *value)
{
  if (value == (const char *) NULL)
    return(MagickFalse);
  if (LocaleCompare(value,"true") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"on") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"yes") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"1") == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s S t r i n g F a l s e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsStringFalse() returns MagickTrue if the value is "false", "off", "no" or
%  "0". Any other string or undefined returns MagickFalse.
%
%  Typically this is used to look at strings (options or artifacts) which
%  has a default value of "true", when it has not been defined.
%
%  The format of the IsStringFalse method is:
%
%      MagickBooleanType IsStringFalse(const char *value)
%
%  A description of each parameter follows:
%
%    o value: Specifies a pointer to a character array.
%
*/
MagickExport MagickBooleanType IsStringFalse(const char *value)
{
  if (value == (const char *) NULL)
    return(MagickFalse);
  if (LocaleCompare(value,"false") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"off") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"no") == 0)
    return(MagickTrue);
  if (LocaleCompare(value,"0") == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i n g I n f o T o S t r i n g                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringInfoToString() converts a string info string to a C string.
%
%  The format of the StringInfoToString method is:
%
%      char *StringInfoToString(const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: the string.
%
*/
MagickExport char *StringInfoToString(const StringInfo *string_info)
{
  char
    *string;

  size_t
    length;

  string=(char *) NULL;
  length=string_info->length;
  if (~length >= (MagickPathExtent-1))
    string=(char *) AcquireQuantumMemory(length+MagickPathExtent,
      sizeof(*string));
  if (string == (char *) NULL)
    return((char *) NULL);
  (void) memcpy(string,(char *) string_info->datum,length*sizeof(*string));
  string[length]='\0';
  return(string);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  F o r m a t L o c a l e S t r i n g                                        %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FormatLocaleString() prints formatted output of a variable argument list to
%  a string buffer in the "C" locale.
%
%  The format of the FormatLocaleString method is:
%
%      ssize_t FormatLocaleString(char *string,const size_t length,
%        const char *format,...)
%
%  A description of each parameter follows.
%
%   o string:  FormatLocaleString() returns the formatted string in this
%     character buffer.
%
%   o length: the maximum length of the string.
%
%   o format:  A string describing the format to use to write the remaining
%     arguments.
%
*/

MagickPrivate ssize_t FormatLocaleStringList(char *magick_restrict string,
  const size_t length,const char *magick_restrict format,va_list operands)
{
  ssize_t
    n;

#if defined(MAGICKCORE_LOCALE_SUPPORT) && defined(MAGICKCORE_HAVE_VSNPRINTF_L)
  {
    locale_t
      locale;

    locale=AcquireCLocale();
    if (locale == (locale_t) NULL)
      n=(ssize_t) vsnprintf(string,length,format,operands);
    else
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
      n=(ssize_t) vsnprintf_l(string,length,format,locale,operands);
#else
      n=(ssize_t) vsnprintf_l(string,length,locale,format,operands);
#endif
  }
#elif defined(MAGICKCORE_HAVE_VSNPRINTF)
#if defined(MAGICKCORE_LOCALE_SUPPORT) && defined(MAGICKCORE_HAVE_USELOCALE)
  {
    locale_t
      locale,
      previous_locale;

    locale=AcquireCLocale();
    if (locale == (locale_t) NULL)
      n=(ssize_t) vsnprintf(string,length,format,operands);
    else
      {
        previous_locale=uselocale(locale);
        n=(ssize_t) vsnprintf(string,length,format,operands);
        uselocale(previous_locale);
      }
  }
#else
  n=(ssize_t) vsnprintf(string,length,format,operands);
#endif
#else
  n=(ssize_t) vsprintf(string,format,operands);
#endif
  if (n < 0)
    string[length-1]='\0';
  return(n);
}

MagickExport ssize_t FormatLocaleString(char *magick_restrict string,
  const size_t length,const char *magick_restrict format,...)
{
  ssize_t
    n;

  va_list
    operands;

  va_start(operands,format);
  n=FormatLocaleStringList(string,length,format,operands);
  va_end(operands);
  return(n);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t P a t h A t t r i b u t e s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPathAttributes() returns attributes (e.g. size of file) about a path.
%
%  The path of the GetPathAttributes method is:
%
%      MagickBooleanType GetPathAttributes(const char *path,void *attributes)
%
%  A description of each parameter follows.
%
%   o  path: the file path.
%
%   o  attributes: the path attributes are returned here.
%
*/
MagickExport MagickBooleanType GetPathAttributes(const char *path,
  void *attributes)
{
  MagickBooleanType
    status;

  if (path == (const char *) NULL)
    {
      errno=EINVAL;
      return(MagickFalse);
    }
  (void) memset(attributes,0,sizeof(struct stat));
  status=/*stat_utf8*/stat(path,(struct stat *) attributes) == 0 ? MagickTrue :
    MagickFalse;
  return(status);
}

MagickExport void *AcquireCriticalMemory(const size_t len)
{
  void
    *memory;

  // Fail if memory request cannot be fulfilled.
  memory=MagickMalloc(len);
  if (memory == (void *) NULL)
    MagickFatalError(ResourceLimitFatalError,MemoryAllocationFailed,
      "ocl: AcquireCriticalMemory");
  return(memory);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t N e x t T o k e n                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNextToken() gets a token from the token stream.  A token is defined as
%  a sequence of characters delimited by whitespace (e.g. clip-path), a
%  sequence delimited with quotes (.e.g "Quote me"), or a sequence enclosed in
%  parenthesis (e.g. rgb(0,0,0)).  GetNextToken() also recognizes these
%  separator characters: ':', '=', ',', and ';'.  GetNextToken() returns the
%  length of the consumed token.
%
%  The format of the GetNextToken method is:
%
%      size_t GetNextToken(const char *magick_restrict start,
%        const char **magick_restrict end,const size_t extent,
%        char *magick_restrict token)
%
%  A description of each parameter follows:
%
%    o start: the start of the token sequence.
%
%    o end: point to the end of the token sequence.
%
%    o extent: maximum extent of the token.
%
%    o token: copy the token to this buffer.
%
*/
MagickExport magick_hot_spot size_t GetNextToken(
  const char *magick_restrict start,const char **magick_restrict end,
  const size_t extent,char *magick_restrict token)
{
  char
    *magick_restrict q;

  const char
    *magick_restrict p;

  double
    value;

  ssize_t
    i;

  assert(start != (const char *) NULL);
  assert(token != (char *) NULL);
  i=0;
  p=start;
  while ((isspace((int) ((unsigned char) *p)) != 0) && (*p != '\0'))
    p++;
  switch (*p)
  {
    case '\0':
      break;
    case '"':
    case '\'':
    case '`':
    case '{':
    {
      char
        escape;

      switch (*p)
      {
        case '"': escape='"'; break;
        case '\'': escape='\''; break;
        case '`': escape='\''; break;
        case '{': escape='}'; break;
        default: escape=(*p); break;
      }
      for (p++; *p != '\0'; p++)
      {
        if ((*p == '\\') && ((*(p+1) == escape) || (*(p+1) == '\\')))
          p++;
        else
          if (*p == escape)
            {
              p++;
              break;
            }
        if (i < (ssize_t) (extent-1))
          token[i++]=(*p);
        if ((size_t) (p-start) >= (extent-1))
          break;
      }
      break;
    }
    case '/':
    {
      if (i < (ssize_t) (extent-1))
        token[i++]=(*p);
      p++;
      if ((*p == '>') || (*p == '/'))
        {
          if (i < (ssize_t) (extent-1))
            token[i++]=(*p);
          p++;
        }
      break;
    }
    default:
    {
      value=/*StringToDouble*/strtod(p,&q);
      (void) value;
      if ((p != q) && (*p != ','))
        {
          for ( ; (p < q) && (*p != ','); p++)
          {
            if (i < (ssize_t) (extent-1))
              token[i++]=(*p);
            if ((size_t) (p-start) >= (extent-1))
              break;
          }
          if (*p == '%')
            {
              if (i < (ssize_t) (extent-1))
                token[i++]=(*p);
              p++;
            }
          break;
        }
      if ((*p != '\0') && (isalpha((int) ((unsigned char) *p)) == 0) &&
          (*p != *DirectorySeparator) && (*p != '#') && (*p != '<'))
        {
          if (i < (ssize_t) (extent-1))
            token[i++]=(*p);
          p++;
          break;
        }
      for ( ; *p != '\0'; p++)
      {
        if (((isspace((int) ((unsigned char) *p)) != 0) || (*p == '=') ||
            (*p == ',') || (*p == ':') || (*p == ';')) && (*(p-1) != '\\'))
          break;
        if ((i > 0) && (*p == '<'))
          break;
        if (i < (ssize_t) (extent-1))
          token[i++]=(*p);
        if (*p == '>')
          break;
        if (*p == '(')
          {
            for (p++; *p != '\0'; p++)
            {
              if (i < (ssize_t) (extent-1))
                token[i++]=(*p);
              if ((*p == ')') && (*(p-1) != '\\'))
                break;
              if ((size_t) (p-start) >= (extent-1))
                break;
            }
            if (*p == '\0')
              break;
          }
        if ((size_t) (p-start) >= (extent-1))
          break;
      }
      break;
    }
  }
  token[i]='\0';
  if (LocaleNCompare(token,"url(#",5) == 0)
    {
      q=strrchr(token,')');
      if (q != (char *) NULL)
        {
          *q='\0';
          (void) memmove(token,token+5,(size_t) (q-token-4));
        }
    }
  while (isspace((int) ((unsigned char) *p)) != 0)
    p++;
  if (end != (const char **) NULL)
    *end=(const char *) p;
  return(p-start+1);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e Q u a n t u m M e m o r y                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireQuantumMemory() returns a pointer to a block of memory at least
%  count * quantum bytes suitably aligned for any use.
%
%  The format of the AcquireQuantumMemory method is:
%
%      void *AcquireQuantumMemory(const size_t count,const size_t quantum)
%
%  A description of each parameter follows:
%
%    o count: the number of objects to allocate contiguously.
%
%    o quantum: the size (in bytes) of each object.
%
*/
MagickExport void *AcquireQuantumMemory(const size_t count,const size_t quantum)
{
  size_t
    size;

  if ((HeapOverflowSanityCheckGetSize(count,quantum,&size) != MagickFalse) ||
      (size > /*GetMaxMemoryRequest()*/LONG_MAX)) // TODO(ocl)
    {
      errno=ENOMEM;
      return(NULL);
    }
  return(AcquireMagickMemory(size));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n t e r p r e t L o c a l e V a l u e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpretLocaleValue() interprets the string as a floating point number in
%  the "C" locale and returns its value as a double. If sentinal is not a null
%  pointer, the method also sets the value pointed by sentinal to point to the
%  first character after the number.
%
%  The format of the InterpretLocaleValue method is:
%
%      double InterpretLocaleValue(const char *value,char **sentinal)
%
%  A description of each parameter follows:
%
%    o value: the string value.
%
%    o sentinal:  if sentinal is not NULL, a pointer to the character after the
%      last character used in the conversion is stored in the location
%      referenced by sentinal.
%
*/
MagickExport double InterpretLocaleValue(const char *magick_restrict string,
  char *magick_restrict *sentinal)
{
  char
    *q;

  double
    value;

  if ((*string == '0') && ((string[1] | 0x20)=='x'))
    value=(double) strtoul(string,&q,16);
  else
    {
#if defined(MAGICKCORE_LOCALE_SUPPORT) && defined(MAGICKCORE_HAVE_STRTOD_L)
      locale_t
        locale;

      locale=AcquireCLocale();
      if (locale == (locale_t) NULL)
        value=strtod(string,&q);
      else
        value=strtod_l(string,&q,locale);
#else
      value=strtod(string,&q);
#endif
    }
  if (sentinal != (char **) NULL)
    *sentinal=q;
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e I m a g e I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireImageInfo() allocates the ImageInfo structure.
%
%  The format of the AcquireImageInfo method is:
%
%      ImageInfo *AcquireImageInfo(void)
%
*/
MagickExport ImageInfo *AcquireImageInfo(void)
{
  ImageInfo
    *image_info;

  image_info=(ImageInfo *) AcquireCriticalMemory(sizeof(*image_info));
  GetImageInfo(image_info);
  return(image_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s i z e Q u a n t u m M e m o r y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResizeQuantumMemory() changes the size of the memory and returns a pointer
%  to the (possibly moved) block.  The contents will be unchanged up to the
%  lesser of the new and old sizes.
%
%  The format of the ResizeQuantumMemory method is:
%
%      void *ResizeQuantumMemory(void *memory,const size_t count,
%        const size_t quantum)
%
%  A description of each parameter follows:
%
%    o memory: A pointer to a memory allocation.
%
%    o count: the number of objects to allocate contiguously.
%
%    o quantum: the size (in bytes) of each object.
%
*/
MagickExport void *ResizeQuantumMemory(void *memory,const size_t count,
  const size_t quantum)
{
  size_t
    size;

  if ((HeapOverflowSanityCheckGetSize(count,quantum,&size) != MagickFalse) ||
      (size > /*GetMaxMemoryRequest()*/LONG_MAX)) // TODO(ocl)
    {
      errno=ENOMEM;
      memory=RelinquishMagickMemory(memory);
      return(NULL);
    }
  return(ResizeMagickMemory(memory,size));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s i z e M a g i c k M e m o r y                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResizeMagickMemory() changes the size of the memory and returns a pointer to
%  the (possibly moved) block.  The contents will be unchanged up to the
%  lesser of the new and old sizes.
%
%  The format of the ResizeMagickMemory method is:
%
%      void *ResizeMagickMemory(void *memory,const size_t size)
%
%  A description of each parameter follows:
%
%    o memory: A pointer to a memory allocation.  On return the pointer
%      may change but the contents of the original allocation will not.
%
%    o size: The new size of the allocated memory.
%
%
*/
MagickExport void *ResizeMagickMemory(void *memory,const size_t size)
{
  void
    *allocation;

  if (memory == (void *) NULL)
    return(AcquireMagickMemory(size));
  allocation=realloc(memory,size);
  if (allocation == (void *) NULL)
    (void) RelinquishMagickMemory(memory);
  return(allocation);
}

#endif /* HAVE_OPENCL */
