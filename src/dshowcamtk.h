#include <windows.h>
#include <initguid.h>

#define NO_DSHOW_STRSAFE
#include <dshow.h>

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
};

#define _ForceRelease(x) {\
    if (x) { \
            ULONG ref = x->Release(); \
            printf("[DEBUG] %p FroceRelease!!, Remaining Ref: %lu\n", x, ref); \
            fflush( stdout );\
            while( ref > 0 ) { ref = x->Release();  printf( "...%lu\n", ref ); fflush( stdout ); } \
            x = NULL; \
        } \
    }

#ifdef DEBUG_MEM
    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #define _LogRelease(name, x) { \
        if (x) { \
            ULONG ref = x->Release(); \
            printf("[DEBUG] %s Release, Remaining RefCount: %lu\n", name, ref); \
            fflush( stdout );\
            x = NULL; \
        } \
    }
    #define _SafeRelease(x) _LogRelease( #x, x )
#else
    #define _SafeRelease(x) { if (x) { x->Release(); x = NULL; } }
#endif
