#ifndef __RGBCONV_H__
#define __RGBCONV_H__

unsigned rgb555rgb(void* in, unsigned sz, void** out,unsigned width, unsigned height );
unsigned rgb565rgb(void* in, unsigned sz, void** out,unsigned width, unsigned height );

#endif // __RGBCONV_H__
