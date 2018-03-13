#ifndef __YUVCONV_H__
#define __YUVCONV_H__

unsigned yuyv2rgb(void* in, unsigned sz, void** out,unsigned width, unsigned height );
unsigned yuvy2rgb(void* in, unsigned sz, void** out, unsigned width, unsigned height );

#endif // __YUVCONV_H__
