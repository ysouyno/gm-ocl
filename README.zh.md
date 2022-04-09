# gm-ocl

[GraphicsMagick](http://www.graphicsmagick.org/) with OpenCL support.

## 为什么会有这个项目？

`GraphicsMagick`（简称`gm`）没有支持`OpenCL`的计划，见官网`FAQ`：“[Are there any plans to use OpenCL or CUDA to use a GPU?](http://www.graphicsmagick.org/FAQ.html#are-there-any-plans-to-use-opencl-or-cuda-to-use-a-gpu)”。

## 基于版本

基于版本：GraphicsMagick-1.3.35

## 支持平台

- [ ] Windows
- [x] Linux

## 使用步骤

1. 编译`gm`时使用添加`--enable-opencl`参数。

``` shellsession
$ ./configure --enable-opencl
```

2. 设置环境变量`MAGICK_OCL_DEVICE`为`true`、`GPU`或`CPU`开启硬件加速。

## 开源协议

从`gm`的`FAQ`：“[How often does GraphicsMagick pick up new code from ImageMagick?](http://www.graphicsmagick.org/FAQ.html#how-often-does-graphicsmagick-pick-up-new-code-from-imagemagick)”中了解到`im`目前采用的是`Apache`协议，该协议侧重于专利保护。

因此在这里附上[`gm`的`Copyright.txt`](Copyright.txt)，[`im`的`LICENSE`](LICENSE.ImageMagick)以及[`im`的`NOTICE`](NOTICE.ImageMagick)，请点击查看详情。
