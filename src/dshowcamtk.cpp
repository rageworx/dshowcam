#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "dshowcamtk.h"

namespace dshowcamtk
{
    HRESULT IsPinConnected(IPin *pPin, BOOL *pResult)
    {
        IPin *pTmp = NULL;
        HRESULT hr = pPin->ConnectedTo(&pTmp);
        if (SUCCEEDED(hr))
        {
            *pResult = TRUE;
        }
        else if (hr == VFW_E_NOT_CONNECTED)
        {
            // The pin is not connected. 
            // This is not an error for our purposes.
            *pResult = FALSE;
            hr = S_OK;
        }

        _SafeRelease(pTmp);
        return hr;
    }

    HRESULT IsPinDirection( IPin* pPin, PIN_DIRECTION dir, BOOL* pResult )
    {
        PIN_DIRECTION pinDir;

        HRESULT hr = pPin->QueryDirection(&pinDir);
        if (SUCCEEDED(hr))
        {
            *pResult = (pinDir == dir);
        }

        return hr;
    }

    HRESULT MatchPin( IPin* pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL* pResult )
    {
        if( pResult == NULL )
        {
            return S_FALSE;
        }

        BOOL bMatch = FALSE;
        BOOL bIsConnected = FALSE;

        HRESULT hr = IsPinConnected(pPin, &bIsConnected);
        if (SUCCEEDED(hr))
        {
            if (bIsConnected == bShouldBeConnected)
            {
                hr = IsPinDirection(pPin, direction, &bMatch);
            }
        }

        if (SUCCEEDED(hr))
        {
            *pResult = bMatch;
        }

        return hr;
    }

    HRESULT FindUnconnectedPin(IBaseFilter* pFilter, PIN_DIRECTION PinDir, IPin** ppPin)
    {
        IEnumPins*  pEnum = NULL;
        IPin*       pPin = NULL;
        BOOL        bFound = FALSE;

        HRESULT hr = pFilter->EnumPins(&pEnum);
        if (FAILED(hr))
        {
            return hr;
        }

        while ( S_OK == pEnum->Next( 1, &pPin, NULL ) )
        {
            hr = MatchPin( pPin, PinDir, FALSE, &bFound );
            if (FAILED(hr))
            {
                _SafeRelease(pPin);
                _SafeRelease(pEnum);

                return hr;
            }

            if (bFound)
            {
                *ppPin = pPin;
                pPin = NULL;
                break;
            }

            _SafeRelease(pPin);
        }

        if ( !bFound )
        {
            hr = VFW_E_NOT_FOUND;
        }

        _SafeRelease(pPin);
        _SafeRelease(pEnum);

        return hr;
    }

    HRESULT ConnectFilters( IGraphBuilder* pGraph,IPin* pOut,IBaseFilter* pDest )
    {
        IPin* pIn = NULL;

        HRESULT hr = FindUnconnectedPin( pDest, PINDIR_INPUT, &pIn );
        if ( SUCCEEDED(hr) && ( pIn != NULL ) )
        {
            hr = pGraph->Connect(pOut, pIn);
            _SafeRelease( pIn );
        }

        return hr;
    }

    HRESULT ConnectFilters( IGraphBuilder* pGraph, IBaseFilter* pSrc, IBaseFilter* pDest )
    {
        IPin* pOut = NULL;

        HRESULT hr = FindUnconnectedPin( pSrc, PINDIR_OUTPUT, &pOut );

        if ( SUCCEEDED(hr) && ( pOut != NULL ) )
        {
            hr = ConnectFilters(pGraph, pOut, pDest);
            _SafeRelease( pOut );
        }
        return hr;
    }

    HRESULT _CopyMediaType( AM_MEDIA_TYPE* pmtTarget, const AM_MEDIA_TYPE* pmtSource )
    {
        if ( pmtSource != pmtTarget )
        {
            *pmtTarget = *pmtSource;

            if ( pmtSource->cbFormat != 0 )
            {
                if ( pmtSource->pbFormat != NULL )
                {
                    pmtTarget->pbFormat = (PBYTE)CoTaskMemAlloc(pmtSource->cbFormat);

                    if ( pmtTarget->pbFormat == NULL )
                    {
                        pmtTarget->cbFormat = 0;
                        return E_OUTOFMEMORY;
                    }
                    else
                    {
                        CopyMemory( (PVOID)pmtTarget->pbFormat,
                                    (PVOID)pmtSource->pbFormat,
                                    pmtTarget->cbFormat );
                    }
                }
                else
                {
                    return S_FALSE;
                }
            }

            if ( pmtTarget->pUnk != NULL )
            {
                pmtTarget->pUnk->AddRef();
            }

            return S_OK;
        }

        return S_FALSE;
    }

    void _FreeMediaType( AM_MEDIA_TYPE& mt )
    {
        if ( mt.cbFormat != 0 )
        {
            CoTaskMemFree((PVOID)mt.pbFormat);
            mt.cbFormat = 0;
            mt.pbFormat = NULL;
        }
        if ( mt.pUnk != NULL )
        {
            _SafeRelease( mt.pUnk );
        }
    }

    void _DeleteMediaType( AM_MEDIA_TYPE *pmt )
    {
        if ( pmt != NULL )
        {
            _FreeMediaType(*pmt);
            CoTaskMemFree(pmt);
        }
    }
    
    const char* GUID2str(GUID guid)
    {
        static char retstr[128] = {0};

        snprintf( retstr, 128,
                  "{%08lX-%04hX-%04hX-%02hhX%02hhX-"
                  "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
                  guid.Data1,
                  guid.Data2,
                  guid.Data3,
                  guid.Data4[0],
                  guid.Data4[1],
                  guid.Data4[2],
                  guid.Data4[3],
                  guid.Data4[4],
                  guid.Data4[5],
                  guid.Data4[6],
                  guid.Data4[7] );

        return retstr;
    }

    void PrintMediaType(int idx, AM_MEDIA_TYPE* pmt)
    {
        if ( pmt != NULL )
        {
            if ( pmt->majortype == MEDIATYPE_Video )
            {
                VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmt->pbFormat;

                RECT rectsrc = pVih->rcSource;
                DWORD bitrate = pVih->dwBitRate;

                LONG width = pVih->bmiHeader.biWidth;
                LONG height = pVih->bmiHeader.biHeight;
                WORD bpp    = pVih->bmiHeader.biBitCount;

                const char* strSub   = "none";

                if ( pmt->subtype == MEDIASUBTYPE_YUYV )
                    strSub = "YUYV";
                else
                if ( pmt->subtype == MEDIASUBTYPE_IYUV )
                    strSub = "IYUV";
                else
                if ( pmt->subtype == MEDIASUBTYPE_YUY2 )
                    strSub = "YUY2";
                else
                if ( pmt->subtype == MEDIASUBTYPE_YVYU )
                    strSub = "YVYU";
                else
                if ( pmt->subtype == MEDIASUBTYPE_MJPG )
                    strSub = "MJPG";
                else
                if ( pmt->subtype == MEDIASUBTYPE_RGB565 )
                    strSub = "RGB565";
                else
                if ( pmt->subtype == MEDIASUBTYPE_RGB555 )
                    strSub = "RGB555";
                else
                if ( pmt->subtype == MEDIASUBTYPE_RGB24 )
                    strSub = "RGB24";
                else
                if ( pmt->subtype == MEDIASUBTYPE_Y8 )
                    strSub = "Y8";
                else
                    strSub = GUID2str( pmt->subtype );

                printf( "[%03d] Media sub type : %s, WxHxBPP = %dx%dx%u\n",
                        idx,
                        strSub,
                        width,
                        height,
                        bpp );
            }
            else
            {
                fprintf( stderr,
                         "Error: media major type is not video ! (%d)\n",
                         pmt->majortype );
            }
        }
    }

    void DisconnectAllPins(ICaptureGraphBuilder2*& pGrp2)
    {
        if ( pGrp2 == NULL )
        {
            return;
        }

        IGraphBuilder* pGraph = NULL;

        if ( FAILED( pGrp2->GetFiltergraph( &pGraph ) ) )
        {
            return;
        }

        IEnumFilters* pFilterEnum = NULL;

        if ( SUCCEEDED( pGraph->EnumFilters( &pFilterEnum ) ) )
        {
            IBaseFilter* pFilter = NULL;

            while ( pFilterEnum->Next( 1, &pFilter, NULL ) == S_OK )
            {
                IEnumPins* pPinEnum = NULL;

                if ( SUCCEEDED( pFilter->EnumPins( &pPinEnum ) ) )
                {
                    IPin* pPin = NULL;

                    while ( pPinEnum->Next( 1, &pPin, NULL ) == S_OK )
                    {
                        IPin* pConnectedPin = NULL;

                        if ( SUCCEEDED( pPin->ConnectedTo( &pConnectedPin ) ) )
                        {
                            pGraph->Disconnect( pPin );
                            pGraph->Disconnect( pConnectedPin );
                            _SafeRelease( pConnectedPin );
                        }
                        
                        _SafeRelease( pPin );
                    }
                    _SafeRelease( pPinEnum );
                }
                _SafeRelease( pFilter );
            }
            _SafeRelease( pFilterEnum );
        }
        _SafeRelease( pGraph );
    }
}; /// of namepsace dshowcamtk.
