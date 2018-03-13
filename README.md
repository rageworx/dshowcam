# dshowcam
 A stand-alone library for capturing continuous images for multiple Windows USB Camera, MinGW-W64.

## Requires 
 * Microsoft Windows 32 or 64bit, supporting DirectShow.
 * MinGW-W64 and M-SYS
 * libraries : kernel32, ole32, strmiids, dxguid, d2d1

## Recommends why MinGW-W64
 * This library may requires to link related in directX.
 * MinGW-W64 supports almost of this relative libraries.

## Supported CODEC
 * All CODECs belong to libuvc (https://int80k.com/libuvc/)
 * YUYV
 * YUVY
 * RGB555
 * RGB565
 * MJPEG
 * ( Y8 -> Not works on DirectShow, DShow cannot handle this, so ignored )

## Status
 * Can handle multiple cameras
 * Can capturing continuous frames from multiple USB cameras.
 * Each different camera configuration availed.
 
## How to build ?
 * Do ```make -f Makefile.mingw``` at your bash.