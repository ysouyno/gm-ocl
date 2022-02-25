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

#endif /* HAVE_OPENCL */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
