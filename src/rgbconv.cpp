#include <cstdio>

#include "rgbconv.h"

unsigned rgb555rgb(void* in, unsigned sz, void** out,unsigned width, unsigned height )
{
    unsigned char* prgb555 = (unsigned char*)in;
    unsigned char* prgb555_end = prgb555 + ( sz );
    unsigned char* prgb = new unsigned char[ ( width * height ) * 3 ];
    unsigned char* prgb_que = prgb;
    unsigned char* prgb_end = prgb + ( ( width * height ) * 3 );

    if ( prgb == NULL )
        return 0;

    while ( ( prgb555 < prgb555_end ) || ( prgb_que < prgb_end ) )
    {
        prgb[0] = ( prgb555[0] & 0x7C00 ) >> 10;
        prgb[1] = ( prgb555[1] & 0x03E0 ) >> 5;
        prgb[2] = ( prgb555[2] & 0x001F );

        prgb_que += 3;
        prgb555  += 2;
    }

    *out = prgb;

    return width * height * 3;
}

unsigned rgb565rgb(void* in, unsigned sz, void** out,unsigned width, unsigned height )
{
    unsigned char* prgb565 = (unsigned char*)in;
    unsigned char* prgb565_end = prgb565 + ( sz );
    unsigned char* prgb = new unsigned char[ ( width * height ) * 3 ];
    unsigned char* prgb_que = prgb;
    unsigned char* prgb_end = prgb + ( ( width * height ) * 3 );

    if ( prgb == NULL )
        return 0;

    while ( ( prgb565 < prgb565_end ) || ( prgb_que < prgb_end ) )
    {
        prgb[0] = ( prgb565[0] & 0xF800 ) >> 11;
        prgb[1] = ( prgb565[1] & 0x07E0 ) >> 5;
        prgb[2] = ( prgb565[2] & 0x001F );

        prgb_que += 3;
        prgb565  += 2;
    }

    *out = prgb;

    return width * height * 3;
}
