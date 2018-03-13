#include <windows.h>
#include <initguid.h>

#define NO_DSHOW_STRSAFE
#include <dshow.h>
#include <qedit.h>

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
    void _DeleteMediaType( AM_MEDIA_TYPE *pmt );

    #define _SafeRelease(x) { if (x) x->Release(); x = NULL; }
};

