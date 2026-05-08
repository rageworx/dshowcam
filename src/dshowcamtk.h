#ifndef __DSHOWCAMTK_H__
#define __DSHOWCAMTK_H__

#include <windows.h>
#include <initguid.h>

#define NO_DSHOW_STRSAFE
#include <dshow.h>

// Y8 GUID : 20203859-0000-0010-8000-00AA00389B71
#ifndef MEDIASUBTYPE_Y8
DEFINE_GUID(MEDIASUBTYPE_Y8,0x20203859,0x0000,0x0010,\
            0x80,0x00,0x000,0xAA,0x00,0x38,0x9B,0x71);
#endif // MEDIASUBTYPE_Y8

#ifndef _MSC_VER
// CLSID_SampleGrabber GUID defines in variable env, 
// So I decide as new one.
#ifndef CLSID_SampleGrabberEx
DEFINE_GUID(CLSID_SampleGrabberEx,0xc1f400a0,0x3f08,0x11d3,\
            0x9f,0x0b,0x00,0x60,0x08,0x03,0x9e,0x37);
#endif // CLSID_SampleGrabber

#ifndef CLSID_NullRenderer
DEFINE_GUID(CLSID_NullRenderer,0xc1f400a4,0x3f08,0x11d3,\
            0x9f,0x0b,0x00,0x60,0x08,0x03,0x9e,0x37);
#endif // CLSID_NullRenderer
#endif /// of _MSC_VER

namespace dshowcamtk
{
    HRESULT IsPinConnected(IPin *pPin, BOOL *pResult);
    HRESULT IsPinDirection( IPin* pPin, PIN_DIRECTION dir, BOOL* pResult );
    HRESULT MatchPin( IPin* pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL* pResult );
    HRESULT FindUnconnectedPin(IBaseFilter* pFilter, PIN_DIRECTION PinDir, IPin** ppPin);
    HRESULT ConnectFilters( IGraphBuilder* pGraph,IPin* pOut,IBaseFilter* pDest );
    HRESULT ConnectFilters( IGraphBuilder* pGraph, IBaseFilter* pSrc, IBaseFilter* pDest );
    HRESULT _CopyMediaType( AM_MEDIA_TYPE* pmtTarget, const AM_MEDIA_TYPE* pmtSource );

    void _FreeMediaType( AM_MEDIA_TYPE& mt );
    void _DeleteMediaType( AM_MEDIA_TYPE* pmt );
    
    const char* GUID2str(GUID guid);
    void PrintMediaType(int idx, AM_MEDIA_TYPE* pmt);
    void DisconnectAllPins(ICaptureGraphBuilder2*& pGrp2);
};

#ifdef DEBUG_MEM
    #define _LogRelease(name, x) { \
        if (x) { \
            ULONG ref = x->Release(); \
            printf("[DEBUG] %s(%s,%u) Release, Remaining RefCount: %lu\n", name, __FILE__, __LINE__, ref); \
            fflush( stdout );\
            x = NULL; \
        } \
    }
    #define _SafeRelease(x) _LogRelease( #x, x )
    #define _ForceRelease(x) {\
    if (x) { \
            ULONG ref = x->Release(); \
            printf("[DEBUG] %s(0x%p,%s,%u) FroceRelease!!, Remaining Ref: %lu\n", #x, x, __FILE__, __LINE__, ref); \
            fflush( stdout );\
            while( ref > 0 ) { ref = x->Release();  printf( "...%lu\n", ref ); fflush( stdout ); } \
            x = NULL; \
        } \
    }    
#else
    #define _SafeRelease(x) { if (x) { x->Release(); x = NULL; } }
    #define _ForceRelease(x) _SafeRelease(x)
#endif

#endif /// of __DSHOWCAMTK_H__