#include <cstdio>
#include <cstdint>

#include "rgbconv.h"

size_t rgb555rgb( void* in, size_t sz, void** out, uint32_t width, uint32_t height )
{
    uint8_t* prgb555 = (uint8_t*)in;
    uint8_t* prgb555_end = prgb555 + ( sz );
    uint8_t* prgb = new uint8_t[ ( width * height ) * 3 ];
    uint8_t* prgb_que = prgb;
    uint8_t* prgb_end = prgb + ( ( width * height ) * 3 );

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

size_t rgb565rgb( void* in, size_t sz, void** out, uint32_t width, uint32_t height )
{
    uint8_t* prgb565 = (uint8_t*)in;
    uint8_t* prgb565_end = prgb565 + ( sz );
    uint8_t* prgb = new uint8_t[ ( width * height ) * 3 ];
    uint8_t* prgb_que = prgb;
    uint8_t* prgb_end = prgb + ( ( width * height ) * 3 );

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
