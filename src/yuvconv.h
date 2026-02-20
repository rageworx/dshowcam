#ifndef __YUVCONV_H__
#define __YUVCONV_H__

size_t yuyv2rgb( void* in, size_t sz, void** out, uint32_t width, uint32_t height );
size_t yuvy2rgb( void* in, size_t sz, void** out, uint32_t width, uint32_t height );

#endif // __YUVCONV_H__
