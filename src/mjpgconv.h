#ifndef __MJPGCONV_H__
#define __MJPGCONV_H__

#ifndef _MSC_VER /// Godamn M$VC not support this codec now.
bool mjpeg2rgb( void* in, size_t sz, void** out, uint32_t width, uint32_t height );
#endif /// of _MSC_VER

#endif // __MJPGCONV_H__
