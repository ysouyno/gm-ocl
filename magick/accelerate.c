/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     AAA     CCCC    CCCC  EEEEE  L      EEEEE  RRRR    AAA   TTTTT  EEEEE   %
%    A   A   C       C      E      L      E      R   R  A   A    T    E       %
%    AAAAA   C       C      EEE    L      EEE    RRRR   AAAAA    T    EEE     %
%    A   A   C       C      E      L      E      R R    A   A    T    E       %
%    A   A    CCCC    CCCC  EEEEE  LLLLL  EEEEE  R  R   A   A    T    EEEEE   %
%                                                                             %
%                                                                             %
%                       MagickCore Acceleration Methods                       %
%                                                                             %
%                              Software Design                                %
%                                  Cristy                                     %
%                               SiuChi Chan                                   %
%                              Guansong Zhang                                 %
%                               January 2010                                  %
%                               Dirk Lemstra                                  %
%                                April 2016                                   %
%                                                                             %
%                                                                             %
%  Copyright @ 2010 ImageMagick Studio LLC, a non-profit organization         %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    https://imagemagick.org/script/license.php                               %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

/*
  Modifications copyright (C) 2022 ysouyno
*/
 
/*
Include declarations.
*/
/* #include "MagickCore/studio.h"
#include "MagickCore/accelerate-private.h"
#include "MagickCore/accelerate-kernels-private.h"
#include "MagickCore/artifact.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/cache-view.h"
#include "MagickCore/color-private.h"
#include "MagickCore/delegate-private.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/gem.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/linked-list.h"
#include "MagickCore/list.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/opencl.h"
#include "MagickCore/opencl-private.h"
#include "MagickCore/option.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/prepress.h"
#include "MagickCore/quantize.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/random_.h"
#include "MagickCore/random-private.h"
#include "MagickCore/registry.h"
#include "MagickCore/resize.h"
#include "MagickCore/resize-private.h"
#include "MagickCore/semaphore.h"
#include "MagickCore/splay-tree.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/token.h" */

#include "magick/studio.h"
#include "magick/opencl-private.h"
#include "magick/opencl.h"

#define MAGICK_MAX(x,y) (((x) >= (y))?(x):(y))
#define MAGICK_MIN(x,y) (((x) <= (y))?(x):(y))

#if defined(HAVE_OPENCL)

// /*
//   Define declarations.
// */
// #define ALIGNED(pointer,type) ((((size_t)(pointer)) & (sizeof(type)-1)) == 0)

// /*
//   Static declarations.
// */
// static const ResizeWeightingFunctionType supportedResizeWeighting[] =
// {
//   BoxWeightingFunction,
//   TriangleWeightingFunction,
//   HannWeightingFunction,
//   HammingWeightingFunction,
//   BlackmanWeightingFunction,
//   CubicBCWeightingFunction,
//   SincWeightingFunction,
//   SincFastWeightingFunction,
//   LastWeightingFunction
// };

/*
  Helper functions.
*/
static MagickBool checkAccelerateCondition(const Image* image)
{
  /* only direct class images are supported */
  if (image->storage_class != DirectClass)
    return(MagickFalse);

  /* check if the image's colorspace is supported */
  if (image->colorspace != RGBColorspace &&
      image->colorspace != sRGBColorspace &&
      // image->colorspace != LinearGRAYColorspace &&
      image->colorspace != GRAYColorspace)
    return(MagickFalse);

  // /* check if the virtual pixel method is compatible with the OpenCL implementation */
  // if ((GetImageVirtualPixelMethod(image) != UndefinedVirtualPixelMethod) &&
  //     (GetImageVirtualPixelMethod(image) != EdgeVirtualPixelMethod))
  //   return(MagickFalse);

  // /* check if the image has mask */
  // if (((image->channels & ReadMaskChannel) != 0) ||
  //     ((image->channels & WriteMaskChannel) != 0) ||
  //     ((image->channels & CompositeMaskChannel) != 0))
  //   return(MagickFalse);

  // if (image->number_channels > 4)
  //   return(MagickFalse);

  // /* check if pixel order is R */
  // if (GetPixelChannelOffset(image,RedPixelChannel) != 0)
  //   return(MagickFalse);

  // if (image->number_channels == 1)
  //   return(MagickTrue);

  // /* check if pixel order is RA */
  // if ((image->number_channels == 2) &&
  //     (GetPixelChannelOffset(image,AlphaPixelChannel) == 1))
  //   return(MagickTrue);

  // if (image->number_channels == 2)
  //   return(MagickFalse);

  // /* check if pixel order is RGB */
  // if ((GetPixelChannelOffset(image,GreenPixelChannel) != 1) ||
  //     (GetPixelChannelOffset(image,BluePixelChannel) != 2))
  //   return(MagickFalse);

  // if (image->number_channels == 3)
  //   return(MagickTrue);

  // /* check if pixel order is RGBA */
  // if (GetPixelChannelOffset(image,AlphaPixelChannel) != 3)
  //   return(MagickFalse);

  return(MagickTrue);
}

static MagickCLEnv getOpenCLEnvironment(ExceptionInfo* exception)
{
  MagickCLEnv
    clEnv;

  clEnv=GetCurrentOpenCLEnv();
  if (clEnv == (MagickCLEnv) NULL)
    return((MagickCLEnv) NULL);

  if (clEnv->enabled == MagickFalse)
    return((MagickCLEnv) NULL);

  if (InitializeOpenCL(clEnv,exception) == MagickFalse)
    return((MagickCLEnv) NULL);

  return(clEnv);
}

/* MagickPrivate */ Image *AccelerateResizeImage(const Image *image,
  const size_t resizedColumns,const size_t resizedRows,
  /* const ResizeFilter *resizeFilter, */ExceptionInfo *exception)
{
  Image
    *filteredImage;

  MagickCLEnv
    clEnv;

  assert(image != NULL);
  assert(exception != (ExceptionInfo *) NULL);

  if (checkAccelerateCondition(image) == MagickFalse)
    return((Image *) NULL);

  // if ((gpuSupportedResizeWeighting(GetResizeFilterWeightingType(
  //        resizeFilter)) == MagickFalse) ||
  //     (gpuSupportedResizeWeighting(GetResizeFilterWindowWeightingType(
  //        resizeFilter)) == MagickFalse))
  //   return((Image *) NULL);

  clEnv=getOpenCLEnvironment(exception);
  if (clEnv == (MagickCLEnv) NULL)
    return((Image *) NULL);

  // filteredImage=ComputeResizeImage(image,clEnv,resizedColumns,resizedRows,
  //   resizeFilter,exception);
  // return(filteredImage);
  return NULL;
}
#endif /* HAVE_OPENCL */
