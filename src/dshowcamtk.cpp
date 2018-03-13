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
            // The pin is not connected. This is not an error for our purposes.
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
                (*ppPin)->AddRef();
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
}; /// of namepsace dshowcamtk.
