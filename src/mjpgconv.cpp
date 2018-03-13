#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mjpgconv.h"

extern "C" {
#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>
};

struct error_mgr
{
    struct jpeg_error_mgr super;
    jmp_buf jmp;
};

static void _error_exit(j_common_ptr dinfo)
{
    struct error_mgr *myerr = (struct error_mgr *)dinfo->err;
    (*dinfo->err->output_message)(dinfo);
    longjmp(myerr->jmp, 1);
}

/* ISO/IEC 10918-1:1993(E) K.3.3. Default Huffman tables used by MJPEG UVC devices
   which don't specify a Huffman table in the JPEG stream. */

static const unsigned char dc_lumi_len[] = \
    {0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};

static const unsigned char dc_lumi_val[] = \
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static const unsigned char dc_chromi_len[] = \
    {0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};

static const unsigned char dc_chromi_val[] = \
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static const unsigned char ac_lumi_len[] = \
    {0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d};

static const unsigned char ac_lumi_val[] = \
    {0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21,
     0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71,
     0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1,
     0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72,
     0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25,
     0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37,
     0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
     0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
     0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
     0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83,
     0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93,
     0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3,
     0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3,
     0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
     0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3,
     0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
     0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1,
     0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};

static const unsigned char ac_chromi_len[] = \
    {0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77};

static const unsigned char ac_chromi_val[] = \
    {0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31,
     0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22,
     0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1,
     0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1,
     0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18,
     0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36,
     0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47,
     0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
     0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
     0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
     0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a,
     0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a,
     0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa,
     0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba,
     0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca,
     0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
     0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
     0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};

#define COPY_HUFF_TABLE(dinfo,tbl,name) do { \
    if(dinfo->tbl == NULL) dinfo->tbl = jpeg_alloc_huff_table((j_common_ptr)dinfo); \
    memcpy(dinfo->tbl->bits, name##_len, sizeof(name##_len)); \
    memset(dinfo->tbl->huffval, 0, sizeof(dinfo->tbl->huffval)); \
    memcpy(dinfo->tbl->huffval, name##_val, sizeof(name##_val)); \
} while(0)

static void insert_huff_tables(j_decompress_ptr dinfo)
{
    COPY_HUFF_TABLE(dinfo, dc_huff_tbl_ptrs[0], dc_lumi);
    COPY_HUFF_TABLE(dinfo, dc_huff_tbl_ptrs[1], dc_chromi);
    COPY_HUFF_TABLE(dinfo, ac_huff_tbl_ptrs[0], ac_lumi);
    COPY_HUFF_TABLE(dinfo, ac_huff_tbl_ptrs[1], ac_chromi);
}

static void init_source (j_decompress_ptr cinfo)
{
}

static boolean fill_input_buffer (j_decompress_ptr cinfo)
{
    ERREXIT(cinfo, JERR_INPUT_EMPTY);
    return TRUE;
}

static void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr* src = (struct jpeg_source_mgr*) cinfo->src;

    if (num_bytes > 0) {
        src->next_input_byte += (size_t) num_bytes;
        src->bytes_in_buffer -= (size_t) num_bytes;
    }
}

static void term_source (j_decompress_ptr cinfo)
{
}

static void jpeg_mem_src (j_decompress_ptr cinfo, void* buffer, long nbytes)
{
    struct jpeg_source_mgr* src;

    if (cinfo->src == NULL)
    {   /* first time for this JPEG object? */
        cinfo->src = \
            (struct jpeg_source_mgr *) \
                (*cinfo->mem->alloc_small)( (j_common_ptr)cinfo,
                                            JPOOL_PERMANENT,
                                            sizeof(struct jpeg_source_mgr) );
    }

    src = (struct jpeg_source_mgr*) cinfo->src;
    src->init_source = init_source;
    src->fill_input_buffer = fill_input_buffer;
    src->skip_input_data = skip_input_data;
    src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->term_source = term_source;
    src->bytes_in_buffer = nbytes;
    src->next_input_byte = (JOCTET*)buffer;
}

unsigned mjpeg2rgb(void* in, unsigned sz, void** out,unsigned width, unsigned height )
{
    struct jpeg_decompress_struct dinfo;
    struct error_mgr jerr;
    size_t lines_read;

    unsigned char* prgb = new unsigned char[ ( width * height ) * 3 ];
    unsigned char* prgb_que = prgb;
    unsigned char* prgb_end = prgb + ( ( width * height ) * 3 );

    if ( prgb == NULL )
        return 0;

    dinfo.err = jpeg_std_error(&jerr.super);
    jerr.super.error_exit = _error_exit;

    if (setjmp(jerr.jmp))
    {
        jpeg_destroy_decompress(&dinfo);
        return false;
    }

    jpeg_create_decompress(&dinfo);
    jpeg_mem_src(&dinfo, in, sz);
    jpeg_read_header(&dinfo, TRUE);

    if (dinfo.dc_huff_tbl_ptrs[0] == NULL)
    {
        /* This frame is missing the Huffman tables: fill in the standard ones */
        insert_huff_tables(&dinfo);
    }

    dinfo.out_color_space = JCS_RGB;
    dinfo.dct_method = JDCT_IFAST;

    jpeg_start_decompress(&dinfo);

    lines_read = 0;

    unsigned out_step = width * 3;

    while (dinfo.output_scanline < dinfo.output_height)
    {
        unsigned char *buffer[1] = {( unsigned char*)prgb + lines_read * out_step };
        int num_scanlines;

        num_scanlines = jpeg_read_scanlines(&dinfo, buffer, 1);
        lines_read += num_scanlines;
    }

    jpeg_finish_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);
	
    return true;
}
