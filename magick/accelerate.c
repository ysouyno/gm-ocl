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
#include "magick/pixel_cache.h"

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

// See pixel.c:InitializePixelChannelMap() in IM
size_t calc_image_number_channels(Image *image)
{
  size_t
   n;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);

  n=0;
  if ((image->colorspace == GRAYColorspace))
    n++;
  else
    n+=3;

  if (image->colorspace == CMYKColorspace)
    n++;
  // No alpha_trait in struct _Image
  if (image->storage_class == PseudoClass)
    n++;
  // No ReadMaskChannel, WriteMaskChannel and CompositeMaskChannel in GM
  // No number_meta_channels in struct _Image

  assert(n < 64);
  return n;
}

/*
  Helper functions.
*/
static MagickBool checkAccelerateCondition(const Image* image)
{
  size_t
    number_channels;

  /* only direct class images are supported */
  if (image->storage_class != DirectClass)
    return(MagickFalse);

  /* check if the image's colorspace is supported */
  if (image->colorspace != RGBColorspace &&
      image->colorspace != sRGBColorspace &&
      // No LinearGRAYColorspace in GM
      // image->colorspace != LinearGRAYColorspace &&
      image->colorspace != GRAYColorspace)
    return(MagickFalse);

  /* check if the virtual pixel method is compatible with the OpenCL implementation */
  if ((GetImageVirtualPixelMethod(image) != UndefinedVirtualPixelMethod) &&
      (GetImageVirtualPixelMethod(image) != EdgeVirtualPixelMethod))
    return(MagickFalse);

  /* check if the image has mask */
  // No ReadMaskChannel, WriteMaskChannel and CompositeMaskChannel in GM
  // see ChannelType definition in magick/image.h
  // if (((image->channels & ReadMaskChannel) != 0) ||
  //     ((image->channels & WriteMaskChannel) != 0) ||
  //     ((image->channels & CompositeMaskChannel) != 0))
  //   return(MagickFalse);

  number_channels = calc_image_number_channels(image);

  if (number_channels > 4)
    return(MagickFalse);

  // /* check if pixel order is R */
  // if (GetPixelChannelOffset(image,RedPixelChannel) != 0)
  //   return(MagickFalse);

  if (number_channels == 1)
    return(MagickTrue);

  // /* check if pixel order is RA */
  // if ((image->number_channels == 2) &&
  //     (GetPixelChannelOffset(image,AlphaPixelChannel) == 1))
  //   return(MagickTrue);

  if (number_channels == 2)
    return(MagickFalse);

  // /* check if pixel order is RGB */
  // if ((GetPixelChannelOffset(image,GreenPixelChannel) != 1) ||
  //     (GetPixelChannelOffset(image,BluePixelChannel) != 2))
  //   return(MagickFalse);

  if (number_channels == 3)
    return(MagickTrue);

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

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     A c c e l e r a t e R e s i z e I m a g e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

static MagickBooleanType resizeHorizontalFilter(MagickCLDevice device,
  cl_command_queue queue,const Image *image,Image *filteredImage,
  cl_mem imageBuffer,cl_uint matte_or_cmyk,cl_uint columns,cl_uint rows,
  cl_mem resizedImageBuffer,cl_uint resizedColumns,cl_uint resizedRows,
  cl_uint filter_type,const FilterInfo *resizeFilter,const double blur,
  cl_mem resizeFilterCubicCoefficients,
  const float xFactor,ExceptionInfo *exception)
{
  cl_kernel
    horizontalKernel;

  cl_int
    status;

  const unsigned int
    workgroupSize = 256;

  float
    resizeFilterScale,
    resizeFilterSupport,
    resizeFilterBlur,
    scale,
    support;

  int
    numCachedPixels,
    resizeFilterType;

  MagickBooleanType
    outputReady;

  size_t
    gammaAccumulatorLocalMemorySize,
    gsize[2],
    i,
    imageCacheLocalMemorySize,
    pixelAccumulatorLocalMemorySize,
    lsize[2],
    totalLocalMemorySize,
    weightAccumulatorLocalMemorySize;

  unsigned int
    chunkSize,
    pixelPerWorkgroup;

  horizontalKernel=NULL;
  outputReady=MagickFalse;

  /*
  Apply filter to resize vertically from image to resize image.
  */
  scale=blur*MAGICK_MAX(1.0/xFactor,1.0);
  support=scale*resizeFilter->support;
  if (support < 0.5)
  {
    /*
    Support too small even for nearest neighbour: Reduce to point
    sampling.
    */
    support=0.5+MagickEpsilon;
    scale=1.0;
  }
  scale=1.0/scale;

  if (resizedColumns < workgroupSize)
  {
    chunkSize=32;
    pixelPerWorkgroup=32;
  }
  else
  {
    chunkSize=workgroupSize;
    pixelPerWorkgroup=workgroupSize;
  }

DisableMSCWarning(4127)
  while(1)
RestoreMSCWarning
  {
    /* calculate the local memory size needed per workgroup */
    numCachedPixels=(int) ceil((pixelPerWorkgroup-1)/xFactor+2*support);
    imageCacheLocalMemorySize=numCachedPixels*sizeof(CLQuantum)*4;
    totalLocalMemorySize=imageCacheLocalMemorySize;

    /* local size for the pixel accumulator */
    pixelAccumulatorLocalMemorySize=chunkSize*sizeof(cl_float4);
    totalLocalMemorySize+=pixelAccumulatorLocalMemorySize;

    /* local memory size for the weight accumulator */
    weightAccumulatorLocalMemorySize=chunkSize*sizeof(float);
    totalLocalMemorySize+=weightAccumulatorLocalMemorySize;

    /* local memory size for the gamma accumulator */
    gammaAccumulatorLocalMemorySize=chunkSize*sizeof(float);
    totalLocalMemorySize+=gammaAccumulatorLocalMemorySize;

    if (totalLocalMemorySize <= device->local_memory_size)
      break;
    else
    {
      pixelPerWorkgroup=pixelPerWorkgroup/2;
      chunkSize=chunkSize/2;
      if ((pixelPerWorkgroup == 0) || (chunkSize == 0))
      {
        /* quit, fallback to CPU */
        goto cleanup;
      }
    }
  }

  // resizeFilterType=(int)GetResizeFilterWeightingType(resizeFilter);
  // resizeWindowType=(int)GetResizeFilterWindowWeightingType(resizeFilter);
  resizeFilterType=filter_type;

  horizontalKernel=AcquireOpenCLKernel(device,"ResizeHorizontalFilter");
  if (horizontalKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.", ".");
    goto cleanup;
  }

  // resizeFilterScale=(float) GetResizeFilterScale(resizeFilter);
  // resizeFilterSupport=(float) GetResizeFilterSupport(resizeFilter);
  // resizeFilterWindowSupport=(float) GetResizeFilterWindowSupport(resizeFilter);
  // resizeFilterBlur=(float) GetResizeFilterBlur(resizeFilter);
  resizeFilterScale=scale;
  resizeFilterSupport=support;
  resizeFilterBlur=blur;

  i=0;
  status =SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_mem),(void*)&imageBuffer);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_uint),(void*)&matte_or_cmyk);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_uint),(void*)&columns);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_uint),(void*)&rows);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_mem),(void*)&resizedImageBuffer);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_uint),(void*)&resizedColumns);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_uint),(void*)&resizedRows);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(float),(void*)&xFactor);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(int),(void*)&resizeFilterType);
  // status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(int),(void*)&resizeWindowType);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(cl_mem),(void*)&resizeFilterCubicCoefficients);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(float),(void*)&resizeFilterScale);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(float),(void*)&resizeFilterSupport);
  // status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(float),(void*)&resizeFilterWindowSupport);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(float),(void*)&resizeFilterBlur);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,imageCacheLocalMemorySize,NULL);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(int),&numCachedPixels);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(unsigned int),&pixelPerWorkgroup);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,sizeof(unsigned int),&chunkSize);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,pixelAccumulatorLocalMemorySize,NULL);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,weightAccumulatorLocalMemorySize,NULL);
  status|=SetOpenCLKernelArg(horizontalKernel,i++,gammaAccumulatorLocalMemorySize,NULL);

  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=(resizedColumns+pixelPerWorkgroup-1)/pixelPerWorkgroup*
    workgroupSize;
  gsize[1]=resizedRows;
  lsize[0]=workgroupSize;
  lsize[1]=1;
  outputReady=EnqueueOpenCLKernel(queue,horizontalKernel,2,
    (const size_t *) NULL,gsize,lsize,image,filteredImage,MagickFalse,
    exception);

cleanup:

  if (horizontalKernel != (cl_kernel) NULL)
    ReleaseOpenCLKernel(horizontalKernel);

  return(outputReady);
}

static MagickBooleanType resizeVerticalFilter(MagickCLDevice device,
  cl_command_queue queue,const Image *image,Image * filteredImage,
  cl_mem imageBuffer,cl_uint matte_or_cmyk,cl_uint columns,cl_uint rows,
  cl_mem resizedImageBuffer,cl_uint resizedColumns,cl_uint resizedRows,
  cl_uint filter_type,const FilterInfo *resizeFilter,const double blur,
  cl_mem resizeFilterCubicCoefficients,
  const float yFactor,ExceptionInfo *exception)
{
  cl_kernel
    verticalKernel;

  cl_int
    status;

  const unsigned int
    workgroupSize = 256;

  float
    resizeFilterScale,
    resizeFilterSupport,
    resizeFilterBlur,
    scale,
    support;

  int
    numCachedPixels,
    resizeFilterType;

  MagickBooleanType
    outputReady;

  size_t
    gammaAccumulatorLocalMemorySize,
    gsize[2],
    i,
    imageCacheLocalMemorySize,
    pixelAccumulatorLocalMemorySize,
    lsize[2],
    totalLocalMemorySize,
    weightAccumulatorLocalMemorySize;

  unsigned int
    chunkSize,
    pixelPerWorkgroup;

  verticalKernel=NULL;
  outputReady=MagickFalse;

  /*
  Apply filter to resize vertically from image to resize image.
  */
  scale=blur*MAGICK_MAX(1.0/yFactor,1.0);
  support=scale*resizeFilter->support;
  if (support < 0.5)
  {
    /*
    Support too small even for nearest neighbour: Reduce to point
    sampling.
    */
    support=0.5+MagickEpsilon;
    scale=1.0;
  }
  scale=1.0/scale;

  if (resizedRows < workgroupSize)
  {
    chunkSize=32;
    pixelPerWorkgroup=32;
  }
  else
  {
    chunkSize=workgroupSize;
    pixelPerWorkgroup=workgroupSize;
  }

DisableMSCWarning(4127)
  while(1)
RestoreMSCWarning
  {
    /* calculate the local memory size needed per workgroup */
    numCachedPixels=(int)ceil((pixelPerWorkgroup-1)/yFactor+2*support);
    imageCacheLocalMemorySize=numCachedPixels*sizeof(CLQuantum)*4;
    totalLocalMemorySize=imageCacheLocalMemorySize;

    /* local size for the pixel accumulator */
    pixelAccumulatorLocalMemorySize=chunkSize*sizeof(cl_float4);
    totalLocalMemorySize+=pixelAccumulatorLocalMemorySize;

    /* local memory size for the weight accumulator */
    weightAccumulatorLocalMemorySize=chunkSize*sizeof(float);
    totalLocalMemorySize+=weightAccumulatorLocalMemorySize;

    /* local memory size for the gamma accumulator */
    gammaAccumulatorLocalMemorySize=chunkSize*sizeof(float);
    totalLocalMemorySize+=gammaAccumulatorLocalMemorySize;

    if (totalLocalMemorySize <= device->local_memory_size)
      break;
    else
    {
      pixelPerWorkgroup=pixelPerWorkgroup/2;
      chunkSize=chunkSize/2;
      if ((pixelPerWorkgroup == 0) || (chunkSize == 0))
      {
        /* quit, fallback to CPU */
        goto cleanup;
      }
    }
  }

  // resizeFilterType=(int)GetResizeFilterWeightingType(resizeFilter);
  // resizeWindowType=(int)GetResizeFilterWindowWeightingType(resizeFilter);
  resizeFilterType=filter_type;

  verticalKernel=AcquireOpenCLKernel(device,"ResizeVerticalFilter");
  if (verticalKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.",".");
    goto cleanup;
  }

  // resizeFilterScale=(float) GetResizeFilterScale(resizeFilter);
  // resizeFilterSupport=(float) GetResizeFilterSupport(resizeFilter);
  // resizeFilterBlur=(float) GetResizeFilterBlur(resizeFilter);
  // resizeFilterWindowSupport=(float) GetResizeFilterWindowSupport(resizeFilter);
  resizeFilterScale=scale;
  resizeFilterSupport=support;
  resizeFilterBlur=blur;

  i=0;
  status =SetOpenCLKernelArg(verticalKernel,i++,sizeof(cl_mem),(void*)&imageBuffer);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(cl_uint),(void*)&matte_or_cmyk);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(cl_uint),(void*)&columns);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(cl_uint),(void*)&rows);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(cl_mem),(void*)&resizedImageBuffer);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(cl_uint),(void*)&resizedColumns);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(cl_uint),(void*)&resizedRows);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(float),(void*)&yFactor);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(int),(void*)&resizeFilterType);
  // status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(int),(void*)&resizeWindowType);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(cl_mem),(void*)&resizeFilterCubicCoefficients);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(float),(void*)&resizeFilterScale);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(float),(void*)&resizeFilterSupport);
  // status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(float),(void*)&resizeFilterWindowSupport);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(float),(void*)&resizeFilterBlur);
  status|=SetOpenCLKernelArg(verticalKernel,i++,imageCacheLocalMemorySize, NULL);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(int), &numCachedPixels);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(unsigned int), &pixelPerWorkgroup);
  status|=SetOpenCLKernelArg(verticalKernel,i++,sizeof(unsigned int), &chunkSize);
  status|=SetOpenCLKernelArg(verticalKernel,i++,pixelAccumulatorLocalMemorySize, NULL);
  status|=SetOpenCLKernelArg(verticalKernel,i++,weightAccumulatorLocalMemorySize, NULL);
  status|=SetOpenCLKernelArg(verticalKernel,i++,gammaAccumulatorLocalMemorySize, NULL);

  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=resizedColumns;
  gsize[1]=(resizedRows+pixelPerWorkgroup-1)/pixelPerWorkgroup*
    workgroupSize;
  lsize[0]=1;
  lsize[1]=workgroupSize;
  outputReady=EnqueueOpenCLKernel(queue,verticalKernel,2,(const size_t *) NULL,
    gsize,lsize,image,filteredImage,MagickFalse,exception);

cleanup:

  if (verticalKernel != (cl_kernel) NULL)
    ReleaseOpenCLKernel(verticalKernel);

  return(outputReady);
}

static Image *ComputeResizeImage(const Image* image,MagickCLEnv clEnv,
  const size_t resizedColumns,const size_t resizedRows,const size_t filter_type,
  const FilterInfo *filter_info,const double blur,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

  cl_mem
    cubicCoefficientsBuffer,
    filteredImageBuffer,
    imageBuffer,
    tempImageBuffer;

  cl_uint
    matte_or_cmyk;

  const double
    *resizeFilterCoefficient;

  float
    coefficientBuffer[7],
    xFactor,
    yFactor;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  MagickSizeType
    length;

  Image
    *filteredImage;

  size_t
    i;

  filteredImage=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  tempImageBuffer=NULL;
  cubicCoefficientsBuffer=NULL;
  outputReady=MagickFalse;

  device=RequestOpenCLDevice(clEnv);
  queue=AcquireOpenCLCommandQueue(device);
  filteredImage=CloneImage(image,resizedColumns,resizedRows,MagickTrue,
    exception);
  if (filteredImage == (Image *) NULL)
    goto cleanup;
  // if (filteredImage->number_channels != image->number_channels)
  //   goto cleanup;
  imageBuffer=GetAuthenticOpenCLBuffer(image,device,exception);
  if (imageBuffer == (cl_mem) NULL)
    goto cleanup;
  filteredImageBuffer=GetAuthenticOpenCLBuffer(filteredImage,device,exception);
  if (filteredImageBuffer == (cl_mem) NULL)
    goto cleanup;

  /* resizeFilterCoefficient=GetResizeFilterCoefficient(resizeFilter);
  for (i = 0; i < 7; i++)
    coefficientBuffer[i]=(float) resizeFilterCoefficient[i];
  cubicCoefficientsBuffer=CreateOpenCLBuffer(device,CL_MEM_COPY_HOST_PTR |
    CL_MEM_READ_ONLY,sizeof(coefficientBuffer),&coefficientBuffer);
  if (cubicCoefficientsBuffer == (cl_mem) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"CreateOpenCLBuffer failed.",".");
    goto cleanup;
  } */

  // No number_channels in struct _Image in GM
  // number_channels=(cl_uint) image->number_channels;
  matte_or_cmyk=(image->matte || image->colorspace == CMYKColorspace)?1:0;
  xFactor=(float) resizedColumns/(float) image->columns;
  yFactor=(float) resizedRows/(float) image->rows;
  if (xFactor > yFactor)
  {
    length=resizedColumns*image->rows*4;
    tempImageBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_WRITE,length*
      sizeof(CLQuantum),(void *) NULL);
    if (tempImageBuffer == (cl_mem) NULL)
    {
      (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
        ResourceLimitWarning,"CreateOpenCLBuffer failed.",".");
      goto cleanup;
    }

    outputReady=resizeHorizontalFilter(device,queue,image,filteredImage,
      imageBuffer,matte_or_cmyk,(cl_uint) image->columns,
      (cl_uint) image->rows,tempImageBuffer,(cl_uint) resizedColumns,
      (cl_uint) image->rows,filter_type,filter_info,blur,
      cubicCoefficientsBuffer,xFactor,exception);
    if (outputReady == MagickFalse)
      goto cleanup;

    outputReady=resizeVerticalFilter(device,queue,image,filteredImage,
      tempImageBuffer,matte_or_cmyk,(cl_uint) resizedColumns,
      (cl_uint) image->rows,filteredImageBuffer,(cl_uint) resizedColumns,
      (cl_uint) resizedRows,filter_type,filter_info,blur,
      cubicCoefficientsBuffer,yFactor,exception);
    if (outputReady == MagickFalse)
      goto cleanup;
  }
  else
  {
    length=image->columns*resizedRows*4;
    tempImageBuffer=CreateOpenCLBuffer(device,CL_MEM_READ_WRITE,length*
      sizeof(CLQuantum),(void *) NULL);
    if (tempImageBuffer == (cl_mem) NULL)
    {
      (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
        ResourceLimitWarning,"CreateOpenCLBuffer failed.",".");
      goto cleanup;
    }

    outputReady=resizeVerticalFilter(device,queue,image,filteredImage,
      imageBuffer,matte_or_cmyk,(cl_uint) image->columns,
      (cl_int) image->rows,tempImageBuffer,(cl_uint) image->columns,
      (cl_uint) resizedRows,filter_type,filter_info,blur,
      cubicCoefficientsBuffer,yFactor,exception);
    if (outputReady == MagickFalse)
      goto cleanup;

    outputReady=resizeHorizontalFilter(device,queue,image,filteredImage,
      tempImageBuffer,matte_or_cmyk,(cl_uint) image->columns,
      (cl_uint) resizedRows,filteredImageBuffer,(cl_uint) resizedColumns,
      (cl_uint) resizedRows,filter_type,filter_info,blur,
      cubicCoefficientsBuffer,xFactor,exception);
    if (outputReady == MagickFalse)
      goto cleanup;
  }

cleanup:

  if (imageBuffer != (cl_mem) NULL)
    ReleaseOpenCLMemObject(imageBuffer);
  if (filteredImageBuffer != (cl_mem) NULL)
    ReleaseOpenCLMemObject(filteredImageBuffer);
  if (tempImageBuffer != (cl_mem) NULL)
    ReleaseOpenCLMemObject(tempImageBuffer);
  if (cubicCoefficientsBuffer != (cl_mem) NULL)
    ReleaseOpenCLMemObject(cubicCoefficientsBuffer);
  if (queue != (cl_command_queue) NULL)
    ReleaseOpenCLCommandQueue(device,queue);
  if (device != (MagickCLDevice) NULL)
    ReleaseOpenCLDevice(device);
  if ((outputReady == MagickFalse) && (filteredImage != (Image *) NULL))
  {
    DestroyImage(filteredImage);
    filteredImage=(Image *) NULL;
  }

  return(filteredImage);
}

MagickPrivate Image *AccelerateResizeImage(const Image *image,
  const size_t resizedColumns,const size_t resizedRows,const size_t filter_type,
  const FilterInfo *filter_info,const double blur,ExceptionInfo *exception)
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

  filteredImage=ComputeResizeImage(image,clEnv,resizedColumns,resizedRows,
    filter_type,filter_info,blur,exception);
  return(filteredImage);
}

static MagickBooleanType scaleFilter(MagickCLDevice device,
  cl_command_queue queue,const Image *image,Image *filteredImage,
  cl_mem imageBuffer,cl_uint matte_or_cmyk,cl_uint columns,cl_uint rows,
  cl_mem scaledImageBuffer,cl_uint scaledColumns,cl_uint scaledRows,
  ExceptionInfo *exception)
{
  cl_kernel
    scaleKernel;

  cl_int
    status;

  const unsigned int
    workgroupSize = 256;

  float
    scale;

  int
    numCachedPixels;

  MagickBooleanType
    outputReady;

  size_t
    gammaAccumulatorLocalMemorySize,
    gsize[2],
    i,
    imageCacheLocalMemorySize,
    pixelAccumulatorLocalMemorySize,
    lsize[2],
    totalLocalMemorySize,
    weightAccumulatorLocalMemorySize;

  unsigned int
    chunkSize,
    pixelPerWorkgroup;

  scaleKernel=NULL;
  outputReady=MagickFalse;

  scale=(float) scaledColumns/columns; // TODO(ocl)

  if (scaledColumns < workgroupSize)
  {
    chunkSize=32;
    pixelPerWorkgroup=32;
  }
  else
  {
    chunkSize=workgroupSize;
    pixelPerWorkgroup=workgroupSize;
  }

DisableMSCWarning(4127)
  while(1)
RestoreMSCWarning
  {
    /* calculate the local memory size needed per workgroup */
    numCachedPixels=(int) ceil((pixelPerWorkgroup-1)/scale+2*(0.5+MagickEpsilon));
    imageCacheLocalMemorySize=numCachedPixels*sizeof(CLQuantum)*4;
    totalLocalMemorySize=imageCacheLocalMemorySize;

    /* local size for the pixel accumulator */
    pixelAccumulatorLocalMemorySize=chunkSize*sizeof(cl_float4);
    totalLocalMemorySize+=pixelAccumulatorLocalMemorySize;

    /* local memory size for the weight accumulator */
    weightAccumulatorLocalMemorySize=chunkSize*sizeof(float);
    totalLocalMemorySize+=weightAccumulatorLocalMemorySize;

    /* local memory size for the gamma accumulator */
    gammaAccumulatorLocalMemorySize=chunkSize*sizeof(float);
    totalLocalMemorySize+=gammaAccumulatorLocalMemorySize;

    if (totalLocalMemorySize <= device->local_memory_size)
      break;
    else
    {
      pixelPerWorkgroup=pixelPerWorkgroup/2;
      chunkSize=chunkSize/2;
      if ((pixelPerWorkgroup == 0) || (chunkSize == 0))
      {
        /* quit, fallback to CPU */
        goto cleanup;
      }
    }
  }

  scaleKernel=AcquireOpenCLKernel(device,"ScaleFilter");
  if (scaleKernel == (cl_kernel) NULL)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"AcquireOpenCLKernel failed.", ".");
    goto cleanup;
  }

  i=0;
  status =SetOpenCLKernelArg(scaleKernel,i++,sizeof(cl_mem),(void*)&imageBuffer);
  status|=SetOpenCLKernelArg(scaleKernel,i++,sizeof(cl_uint),(void*)&matte_or_cmyk);
  status|=SetOpenCLKernelArg(scaleKernel,i++,sizeof(cl_uint),(void*)&columns);
  status|=SetOpenCLKernelArg(scaleKernel,i++,sizeof(cl_uint),(void*)&rows);
  status|=SetOpenCLKernelArg(scaleKernel,i++,sizeof(cl_mem),(void*)&scaledImageBuffer);
  status|=SetOpenCLKernelArg(scaleKernel,i++,sizeof(cl_uint),(void*)&scaledColumns);
  status|=SetOpenCLKernelArg(scaleKernel,i++,sizeof(cl_uint),(void*)&scaledRows);
  status|=SetOpenCLKernelArg(scaleKernel,i++,sizeof(float),(void*)&scale);
  status|=SetOpenCLKernelArg(scaleKernel,i++,imageCacheLocalMemorySize,NULL);
  status|=SetOpenCLKernelArg(scaleKernel,i++,sizeof(int),&numCachedPixels);
  status|=SetOpenCLKernelArg(scaleKernel,i++,sizeof(unsigned int),&pixelPerWorkgroup);
  status|=SetOpenCLKernelArg(scaleKernel,i++,sizeof(unsigned int),&chunkSize);
  status|=SetOpenCLKernelArg(scaleKernel,i++,pixelAccumulatorLocalMemorySize,NULL);
  status|=SetOpenCLKernelArg(scaleKernel,i++,weightAccumulatorLocalMemorySize,NULL);
  status|=SetOpenCLKernelArg(scaleKernel,i++,gammaAccumulatorLocalMemorySize,NULL);

  if (status != CL_SUCCESS)
  {
    (void) OpenCLThrowMagickException(device,exception,GetMagickModule(),
      ResourceLimitWarning,"SetOpenCLKernelArg failed.",".");
    goto cleanup;
  }

  gsize[0]=(scaledColumns+pixelPerWorkgroup-1)/pixelPerWorkgroup*
    workgroupSize;
  gsize[1]=scaledRows;
  lsize[0]=workgroupSize;
  lsize[1]=1;
  outputReady=EnqueueOpenCLKernel(queue,scaleKernel,2,
    (const size_t *) NULL,gsize,lsize,image,filteredImage,MagickFalse,
    exception);

cleanup:

  if (scaleKernel != (cl_kernel) NULL)
    ReleaseOpenCLKernel(scaleKernel);

  return(outputReady);
}

static Image *ComputeScaleImage(const Image* image,MagickCLEnv clEnv,
  const size_t scaledColumns,const size_t scaledRows,ExceptionInfo *exception)
{
  cl_command_queue
    queue;

  cl_mem
    filteredImageBuffer,
    imageBuffer;

  cl_uint
    matte_or_cmyk;

  MagickBooleanType
    outputReady;

  MagickCLDevice
    device;

  MagickSizeType
    length;

  Image
    *filteredImage;

  size_t
    i;

  filteredImage=NULL;
  imageBuffer=NULL;
  filteredImageBuffer=NULL;
  outputReady=MagickFalse;

  device=RequestOpenCLDevice(clEnv);
  queue=AcquireOpenCLCommandQueue(device);
  filteredImage=CloneImage(image,scaledColumns,scaledRows,MagickTrue,
    exception);
  if (filteredImage == (Image *) NULL)
    goto cleanup;
  imageBuffer=GetAuthenticOpenCLBuffer(image,device,exception);
  if (imageBuffer == (cl_mem) NULL)
    goto cleanup;
  filteredImageBuffer=GetAuthenticOpenCLBuffer(filteredImage,device,exception);
  if (filteredImageBuffer == (cl_mem) NULL)
    goto cleanup;

  matte_or_cmyk=(image->matte || image->colorspace == CMYKColorspace)?1:0;

  outputReady=scaleFilter(device,queue,image,filteredImage,
    imageBuffer,matte_or_cmyk,(cl_uint) image->columns,
    (cl_uint) image->rows,filteredImageBuffer,(cl_uint) scaledColumns,
    (cl_uint) scaledRows,exception);
  if (outputReady == MagickFalse)
    goto cleanup;

cleanup:

  if (imageBuffer != (cl_mem) NULL)
    ReleaseOpenCLMemObject(imageBuffer);
  if (filteredImageBuffer != (cl_mem) NULL)
    ReleaseOpenCLMemObject(filteredImageBuffer);
  if (queue != (cl_command_queue) NULL)
    ReleaseOpenCLCommandQueue(device,queue);
  if (device != (MagickCLDevice) NULL)
    ReleaseOpenCLDevice(device);
  if ((outputReady == MagickFalse) && (filteredImage != (Image *) NULL))
  {
    DestroyImage(filteredImage);
    filteredImage=(Image *) NULL;
  }

  return(filteredImage);
}

MagickPrivate Image *AccelerateScaleImage(const Image *image,
  const size_t scaledColumns,const size_t scaledRows,
  ExceptionInfo *exception)
{
  Image
    *filteredImage;

  MagickCLEnv
    clEnv;

  assert(image != NULL);
  assert(exception != (ExceptionInfo *) NULL);

  if (checkAccelerateCondition(image) == MagickFalse)
    return((Image *) NULL);

  clEnv=getOpenCLEnvironment(exception);
  if (clEnv == (MagickCLEnv) NULL)
    return((Image *) NULL);

  filteredImage=ComputeScaleImage(image,clEnv,scaledColumns,scaledRows,
    exception);
  return(filteredImage);
}

#endif /* HAVE_OPENCL */
