#include <cstdio>

#include "yuvconv.h"

#define sat( _x_ )  (unsigned char)( (int)_x_ >= 255 ? 255 : ( (int)_x_ < 0 ? 0 : (int)_x_ ))

#define YUYV2RGB_2(pyuv, prgb) { \
    float r = 1.402f * ((pyuv)[3]-128); \
    float g = -0.34414f * ((pyuv)[1]-128) - 0.71414f * ((pyuv)[3]-128); \
    float b = 1.772f * ((pyuv)[1]-128); \
    (prgb)[0] = sat((pyuv[0] + r)); \
    (prgb)[1] = sat((pyuv[0] + g)); \
    (prgb)[2] = sat((pyuv[0] + b)); \
    (prgb)[3] = sat((pyuv[2] + r)); \
    (prgb)[4] = sat((pyuv[2] + g)); \
    (prgb)[5] = sat((pyuv[2] + b)); \
    }

#define IYUYV2RGB_2(pyuv, prgb) { \
    int r = (22987 * ((pyuv)[3] - 128)) >> 14; \
    int g = (-5636 * ((pyuv)[1] - 128) - 11698 * ((pyuv)[3] - 128)) >> 14; \
    int b = (29049 * ((pyuv)[1] - 128)) >> 14; \
    (prgb)[0] = sat((*(pyuv) + r)); \
    (prgb)[1] = sat((*(pyuv) + g)); \
    (prgb)[2] = sat((*(pyuv) + b)); \
    (prgb)[3] = sat(((pyuv)[2] + r)); \
    (prgb)[4] = sat(((pyuv)[2] + g)); \
    (prgb)[5] = sat(((pyuv)[2] + b)); \
    }

#define IYUYV2RGB_4(pyuv, prgb) IYUYV2RGB_2(pyuv, prgb); IYUYV2RGB_2(pyuv + 4, prgb + 6);
#define IYUYV2RGB_8(pyuv, prgb) IYUYV2RGB_4(pyuv, prgb); IYUYV2RGB_4(pyuv + 8, prgb + 12);
#define IYUYV2RGB_16(pyuv, prgb) IYUYV2RGB_8(pyuv, prgb); IYUYV2RGB_8(pyuv + 16, prgb + 24);

unsigned yuyv2rgb(void* in, unsigned sz, void** out,unsigned width, unsigned height )
{
    unsigned char* pyuv = (unsigned char*)in;
    unsigned char* pyuv_end = pyuv + ( sz );
    unsigned char* prgb = new unsigned char[ ( width * height ) * 3 ];
    unsigned char* prgb_que = prgb;
    unsigned char* prgb_end = prgb + ( ( width * height ) * 3 );

    if ( prgb == NULL )
        return 0;

    while ( ( pyuv < pyuv_end ) || ( prgb_que < prgb_end ) )
    {
        IYUYV2RGB_8(pyuv, prgb_que);

        prgb_que += ( 3 * 8 );
        pyuv     += ( 2 * 8 );
    }

    *out = prgb;

    return width * height * 3;
}

#define IUYVY2RGB_2(pyuv, prgb) { \
    int r = (22987 * ((pyuv)[2] - 128)) >> 14; \
    int g = (-5636 * ((pyuv)[0] - 128) - 11698 * ((pyuv)[2] - 128)) >> 14; \
    int b = (29049 * ((pyuv)[0] - 128)) >> 14; \
    (prgb)[0] = sat((pyuv)[1] + r); \
    (prgb)[1] = sat((pyuv)[1] + g); \
    (prgb)[2] = sat((pyuv)[1] + b); \
    (prgb)[3] = sat((pyuv)[3] + r); \
    (prgb)[4] = sat((pyuv)[3] + g); \
    (prgb)[5] = sat((pyuv)[3] + b); \
    }
#define IUYVY2RGB_16(pyuv, prgb) IUYVY2RGB_8(pyuv, prgb); IUYVY2RGB_8(pyuv + 16, prgb + 24);
#define IUYVY2RGB_8(pyuv, prgb) IUYVY2RGB_4(pyuv, prgb); IUYVY2RGB_4(pyuv + 8, prgb + 12);
#define IUYVY2RGB_4(pyuv, prgb) IUYVY2RGB_2(pyuv, prgb); IUYVY2RGB_2(pyuv + 4, prgb + 6);

unsigned yuvy2rgb(void* in, unsigned sz, void** out, unsigned width, unsigned height )
{
    unsigned char* pyuv = (unsigned char*)in;
    unsigned char* pyuv_end = pyuv + ( sz );
    unsigned char* prgb = new unsigned char[ ( width * height ) * 3 ];
    unsigned char* prgb_que = prgb;
    unsigned char* prgb_end = prgb + ( ( width * height ) * 3 );

    if ( prgb == NULL )
        return 0;

    while ( ( pyuv < pyuv_end ) || ( prgb_que < prgb_end ) )
    {
        IUYVY2RGB_8(pyuv, prgb_que);

        prgb_que += ( 3 * 8 );
        pyuv     += ( 2 * 8 );
    }

    *out = prgb;

    return width * height *3;
}
