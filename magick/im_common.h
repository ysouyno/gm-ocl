/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/script/license.php

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private methods for internal threading.
    (MagickThreadType)
  MagickCore method attributes.
    (MagickPathExtent)
  MagickCore string methods.
    (StringInfo, GetEnvironmentValue(), CopyMagickString(), DestroyString(),
    IsStringTrue(), ConstantString())
  MagickCore locale methods.
    (FormatLocaleString())
  MagickCore private application programming interface declarations.
    (magick_restrict)
  MagickCore method attributes.
    (magick_attribute, magick_hot_spot, magick_alloc_sizes)
  MagickCore utility methods.
    (GetPathAttributes())
  MagickCore token methods.
    (GetNextToken())
  MagickCore memory methods.
    (AcquireQuantumMemory(), HeapOverflowSanityCheck(), ResizeQuantumMemory())
  MagickCore private string methods.
    (StringToInteger())
  MagickCore image private methods.
    (MagickMax, MagickMin)
  MagickCore image methods.
    (AcquireImageInfo())
  MagickCore private methods for internal threading.
    (GetMagickThreadId())
  MagickCore types.
    (QuantumRange, QuantumScale)
*/

/*
  Modifications copyright (C) 2022 ysouyno
*/

#ifndef IM_COMMON_H
#define IM_COMMON_H

// #include "MagickCore/magick-config.h"
#include "magick/semaphore.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(HAVE_OPENCL)

typedef MagickBool MagickBooleanType;

#if !defined(magick_restrict)
# if !defined(_magickcore_restrict)
#  define magick_restrict restrict
# else
#  define magick_restrict _magickcore_restrict
# endif
#endif

#if defined(MAGICKCORE_HAVE___ATTRIBUTE__)
#  define magick_aligned(x,y)  x __attribute__((aligned(y)))
#  define magick_attribute  __attribute__
#  define magick_unused(x)  magick_unused_ ## x __attribute__((unused))
#  define magick_unreferenced(x)  /* nothing */
#elif defined(MAGICKCORE_WINDOWS_SUPPORT) && !defined(__CYGWIN__)
#  define magick_aligned(x,y)  __declspec(align(y)) x
#  define magick_attribute(x)  /* nothing */
#  define magick_unused(x) x
#  define magick_unreferenced(x) (x)
#else
#  define magick_aligned(x,y)  /* nothing */
#  define magick_attribute(x)  /* nothing */
#  define magick_unused(x) x
#  define magick_unreferenced(x)  /* nothing */
#endif

#if defined(__clang__) || (((__GNUC__) > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3)))
#  define magick_cold_spot  __attribute__((__cold__))
#  define magick_hot_spot  __attribute__((__hot__))
#else
#  define magick_cold_spot
#  define magick_hot_spot
#endif

#if !defined(__clang__) && (((__GNUC__) > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3)))
#  define magick_alloc_size(x)  __attribute__((__alloc_size__(x)))
#  define magick_alloc_sizes(x,y)  __attribute__((__alloc_size__(x,y)))
#else
#  define magick_alloc_size(x)  /* nothing */
#  define magick_alloc_sizes(x,y)  /* nothing */
#endif

#define MagickMax(x,y) (((x) > (y)) ? (x) : (y))
#define MagickMin(x,y) (((x) < (y)) ? (x) : (y))

// TODO
#define MAGICKCORE_QUANTUM_DEPTH 8

// TODO
#if (MAGICKCORE_QUANTUM_DEPTH == 8)
// #define MaxColormapSize  256UL
// #define MaxMap  255UL

#if defined(MAGICKCORE_HDRI_SUPPORT)
typedef MagickFloatType Quantum;
#define QuantumRange  255.0
#define QuantumFormat  "%g"
#else
typedef unsigned char Quantum;
#define QuantumRange  ((Quantum) 255)
#define QuantumFormat  "%u"
#endif
#elif (MAGICKCORE_QUANTUM_DEPTH == 16)
#define MaxColormapSize  65536UL
#define MaxMap  65535UL

#if defined(MAGICKCORE_HDRI_SUPPORT)
typedef MagickFloatType Quantum;
#define QuantumRange  65535.0f
#define QuantumFormat  "%g"
#else
typedef unsigned short Quantum;
#define QuantumRange  ((Quantum) 65535)
#define QuantumFormat  "%u"
#endif
#elif (MAGICKCORE_QUANTUM_DEPTH == 32)
#define MaxColormapSize  65536UL
#define MaxMap  65535UL

#if defined(MAGICKCORE_HDRI_SUPPORT)
typedef MagickDoubleType Quantum;
#define QuantumRange  4294967295.0
#define QuantumFormat  "%g"
#else
typedef unsigned int Quantum;
#define QuantumRange  ((Quantum) 4294967295)
#define QuantumFormat  "%u"
#endif
#elif (MAGICKCORE_QUANTUM_DEPTH == 64)
#define MAGICKCORE_HDRI_SUPPORT  1
#define MaxColormapSize  65536UL
#define MaxMap  65535UL

typedef MagickDoubleType Quantum;
#define QuantumRange  18446744073709551615.0
#define QuantumFormat  "%g"
#else
#error "MAGICKCORE_QUANTUM_DEPTH must be one of 8, 16, 32, or 64"
#endif
#define MagickEpsilon  1.0e-12
#define MagickMaximumValue  1.79769313486231570E+308
#define MagickMinimumValue   2.22507385850720140E-308
#define MagickStringify(macro_or_string)  MagickStringifyArg(macro_or_string)
#define MagickStringifyArg(contents)  #contents
#define QuantumScale  ((double) 1.0/(double) QuantumRange)

// Copy form GM magick_compat.c
#define AcquireMagickMemory(memory) malloc(memory)

/*
  ImageMagick compatibility definitions
*/
#define MagickSizeType magick_int64_t // TODO

#define MagickPrivate

// TODO
#if defined(MAGICKCORE_THREAD_SUPPORT)
typedef pthread_t MagickThreadType;
#elif defined(MAGICKCORE_WINDOWS_SUPPORT)
typedef DWORD MagickThreadType;
#else
typedef pid_t MagickThreadType;
#endif

#if !defined(MagickPathExtent)
#  define MagickPathExtent  4096  /* always >= max(PATH_MAX,4096) */
#endif

typedef struct _StringInfo
{
  char
    *path;

  unsigned char
    *datum;

  size_t
    length,
    signature;

  char
    *name;
} StringInfo;

// Copy form GM magick_compat.c
void *RelinquishMagickMemory(void *);

char *GetEnvironmentValue(const char *);

extern ssize_t
  FormatLocaleString(char *magick_restrict,const size_t,
    const char *magick_restrict,...)
    magick_attribute((__format__ (__printf__,3,4)));

extern MagickBooleanType
  GetPathAttributes(const char *,void *);

extern void
  *AcquireCriticalMemory(const size_t);

extern MagickExport size_t
  CopyMagickString(char *magick_restrict,const char *magick_restrict,
    const size_t) magick_attribute((__nonnull__));

extern MagickExport char
  *ConstantString(const char *),
  *DestroyString(char *);

extern MagickExport MagickBooleanType
  IsStringTrue(const char *) magick_attribute((__pure__));

extern MagickExport size_t
  GetNextToken(const char *magick_restrict,const char **magick_restrict,
    const size_t,char *magick_restrict) magick_hot_spot;

static inline MagickBooleanType HeapOverflowSanityCheck(
  const size_t count,const size_t quantum)
{
  if ((count == 0) || (quantum == 0))
    return(MagickTrue);
  if (quantum != ((count*quantum)/count))
    {
      errno=ENOMEM;
      return(MagickTrue);
    }
  return(MagickFalse);
}

extern MagickExport void
  *AcquireQuantumMemory(const size_t,const size_t);

static inline int StringToInteger(const char *magick_restrict value)
{
  return((int) strtol(value,(char **) NULL,10));
}

extern MagickExport ImageInfo
  *AcquireImageInfo(void);

static inline MagickThreadType GetMagickThreadId(void)
{
#if defined(MAGICKCORE_THREAD_SUPPORT)
  return(pthread_self());
#elif defined(MAGICKCORE_WINDOWS_SUPPORT)
  return(GetCurrentThreadId());
#else
  return(getpid());
#endif
}

extern MagickExport void
  *ResizeQuantumMemory(void *,const size_t,const size_t)
    magick_attribute((__malloc__)) magick_alloc_sizes(2,3);

#endif /* HAVE_OPENCL */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
