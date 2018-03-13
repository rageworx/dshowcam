#ifndef __MJPGCONV_H__
#define __MJPGCONV_H__

#ifndef _MSC_VER /// Godamn M$VC not support this codec now.
unsigned mjpeg2rgb(void* in, unsigned sz, void** out,unsigned width, unsigned height );
#endif /// of _MSC_VER

#endif // __MJPGCONV_H__
