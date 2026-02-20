#ifndef __RGBCONV_H__
#define __RGBCONV_H__

size_t rgb555rgb( void* in, size_t sz, void** out, uint32_t width, uint32_t height );
size_t rgb565rgb( void* in, size_t sz, void** out, uint32_t width, uint32_t height );

#endif // __RGBCONV_H__
