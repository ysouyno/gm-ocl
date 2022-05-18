# gm-ocl

[GraphicsMagick](http://www.graphicsmagick.org/) with OpenCL support.

## Why this project?

`GraphicsMagick` (short for `gm`) has no plans to support `OpenCL`, see the official website `FAQ`: "[Are there any plans to use OpenCL or CUDA to use a GPU?](http://www.graphicsmagick.org/FAQ.html#are-there-any-plans-to-use-opencl-or-cuda-to-use-a-gpu)”.

## Version based

Based on version: GraphicsMagick-1.3.35

## Supported platforms

- [x] Windows
- [x] Linux

## Steps for usage

1. Use the newly added `--enable-opencl` parameter when compiling `gm`.

``` shellsession
$ ./configure --enable-opencl
````

2. Set the environment variable `MAGICK_OCL_DEVICE` to `true`, `GPU` or `CPU` to enable hardware acceleration.

## License

From `FAQ` of `gm`: "[How often does GraphicsMagick pick up new code from ImageMagick?](http://www.graphicsmagick.org/FAQ.html#how-often-does-graphicsmagick-pick-up-new-code-from-imagemagick)" to learn that `im` currently uses the `Apache` protocol, which focuses on patent protection.

So here is attached [`Copyright.txt` for `gm`](Copyright.txt), [`LICENSE` for `im`](LICENSE.ImageMagick) and [`NOTICE` for `im`](NOTICE.ImageMagick ), click to view details.

## Benchmark

|               | Iterations | Zoom in/out | Disable OpenCL | Enable OpenCL |
| :--           |        :-: |         :-: | :-:            | :-:           |
| ResizeImage() |        100 |         50% | 6.36s          | 8.05s         |
| ResizeImage() |        100 |        200% | 26.6s          | 18.7s         |
| ScaleImage()  |        100 |         50% | 5.89s          | 5.15s         |
| ScaleImage()  |        100 |        200% | 23.6s          | 15.8s         |
