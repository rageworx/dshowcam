#include <windows.h>
#include <dshow.h>
#include "dshowcam.h"

////////////////////////////////////////////////////////////////////////////////

#define WM_CAPTURESIG       ( WM_USER + 1828 )

////////////////////////////////////////////////////////////////////////////////

static bool _COMOBJ_INIT = false;
static int  _COMOBJ_REFCOUNT = 0;

static HINSTANCE    _parent_instance = NULL;
static HWND	        _parent_window = NULL;
static HWND         _hiddenWindow = NULL;

static void _Initialize_DSHOWCOMOBJ();
static void _Finalize_DSHOWCOMOBJ();
static void _Initialize_HiddenWindow();
static void _Finalize_HiddenWindow();

static void _FreeMediaType(AM_MEDIA_TYPE& mt);
static void _DeleteMediaType(AM_MEDIA_TYPE *pmt);

LRESULT WINAPI _HiddenWinProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

////////////////////////////////////////////////////////////////////////////////

class DxDShowProperties
{
    public:
        DxDShowProperties():
            pGraphBuilder( NULL ),
            pCaptureGraphBuilder2( NULL ),
            pMediaControl( NULL ),
            pDeviceFilter( NULL ),
            pVmr( NULL ),
            pVmrWinCtrl( NULL ),
            pConfig( NULL )
        {
            // Initialized.
        }

    public:
        ~DxDShowProperties()
        {
            if ( pMediaControl != NULL )
            {
                pMediaControl->Stop();
                pMediaControl->Release();
                pMediaControl = NULL;
            }

            if ( pVmrWinCtrl != NULL )
            {
                pVmrWinCtrl->Release();
                pVmrWinCtrl = NULL;
            }

            if ( pVmr != NULL )
            {
                pVmr->Release();
                pVmr = NULL;
            }

            if ( pDeviceFilter != NULL )
            {
                pDeviceFilter->Release();
                pDeviceFilter = NULL;
            }

            if ( pCaptureGraphBuilder2 != NULL )
            {
                pCaptureGraphBuilder2->Release();
                pCaptureGraphBuilder2 = NULL;
            }

            if ( pGraphBuilder != NULL )
            {
                pGraphBuilder->Release();
                pGraphBuilder = NULL;
            }

            if ( pConfig != NULL )
            {
                pConfig->Release();
                pConfig = NULL;
            }
        }

    public:
        IGraphBuilder*          pGraphBuilder;
        ICaptureGraphBuilder2*  pCaptureGraphBuilder2;
        IMediaControl*          pMediaControl;
        IBaseFilter*            pDeviceFilter;
        IBaseFilter*            pVmr;
        IVMRWindowlessControl*  pVmrWinCtrl;
        IAMStreamConfig*        pConfig;
};

////////////////////////////////////////////////////////////////////////////////

static void _Initialize_DSHOWCOMOBJ()
{
    if ( _COMOBJ_INIT == false )
    {
        CoInitialize(NULL);
        _COMOBJ_REFCOUNT++;
    }
}

static void _Finalize_DSHOWCOMOBJ()
{
    if ( ( _COMOBJ_REFCOUNT > 0 ) && ( _COMOBJ_INIT == true ) )
    {
        if ( _COMOBJ_REFCOUNT == 1 )
        {
            CoUninitialize();
            _COMOBJ_INIT = false;
        }

        _COMOBJ_REFCOUNT--;
    }
}

static void _Initialize_HiddenWindow()
{
    if ( _hiddenWindow == NULL )
    {
		const wchar_t winclsnm[] = L"hiddendshowin";

		// Automated parent instance and window.
		if ( _parent_window == NULL )
        {
            _parent_window = GetWindow( NULL, GW_OWNER );
        }

        if ( ( _parent_instance == NULL ) && ( _parent_window != NULL ) )
        {
            _parent_instance = (HINSTANCE)GetWindowLong( _parent_window,
                                                         GWLP_HINSTANCE );
        }

		// Make a hidden win32 empty window for listen message.
		WNDCLASSEX wincl    = { 0 };
		wincl.cbSize        = sizeof ( WNDCLASSEX );
		wincl.hInstance     = _parent_instance;
		wincl.lpszClassName = winclsnm;
		wincl.lpfnWndProc   = _HiddenWinProc;

		if (RegisterClassEx ( &wincl ))
		{
			_hiddenWindow = CreateWindowEx ( 0,
                                             winclsnm,
                                             L"hidden dshow control window",
                                             WS_CHILDWINDOW,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             10,
                                             10,
                                             _parent_window,
                                             NULL,
                                             _parent_instance,
                                             NULL );

			if ( _hiddenWindow != NULL )
			{
			    // Make window goes hidden.
				ShowWindow( _hiddenWindow, SW_HIDE );
			}
#ifdef DEBUG
			else
			{
				DWORD le = GetLastError();
				printf( "Error ID:%u(0x%X)\n", le, le );
			}
#endif
		}
    }
}

static void _Finalize_HiddenWindow()
{
    if ( _hiddenWindow != NULL )
    {
        //SendMessage( _hiddenWindow, WM_QUIT, 0, 0 );
        if ( CloseWindow( _hiddenWindow ) == TRUE )
        {
            _hiddenWindow = NULL;
        }
    }
}

static void _FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }

    if (mt.pUnk != NULL)
    {
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

static void _DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt != NULL)
    {
        _FreeMediaType(*pmt);
        CoTaskMemFree(pmt);
    }
}

LRESULT WINAPI _HiddenWinProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
		case WM_CAPTURESIG:
			{
			}
			break;
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


DShowCamera::DShowCamera()
 : dxdshowprop( NULL ),
   bConfigured( false )
{
    _Initialize_DSHOWCOMOBJ();
}

DShowCamera::~DShowCamera()
{
    _Finalize_DSHOWCOMOBJ();
}

void DShowCamera::EnermateDevice( DeviceInfos* retDeviceInfos )
{
    if ( _COMOBJ_INIT == true )
    {
        camDevInfo.clear();

        ICreateDevEnum* pCDevEnum = NULL;
        IEnumMoniker*   pEnumMoniker = NULL;

        HRESULT hr = \
        CoCreateInstance( CLSID_SystemDeviceEnum,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ICreateDevEnum,
                          (PVOID*)&pCDevEnum );

        if ( SUCCEEDED(hr) && ( pCDevEnum != NULL ) )
        {
            pCDevEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory,
                                              &pEnumMoniker,
                                              0 );

            if (pEnumMoniker == NULL)
            {
                pCDevEnum->Release();
                return;
            }
        }

        pEnumMoniker->Reset();

        IMoniker* pMoniker = NULL;
        ULONG     nFetched = 0;

        while ( pEnumMoniker->Next( 1, &pMoniker, &nFetched ) == S_OK )
        {
            bool newCDIavailed = false;
            CameraDeviceInfo newCDI;

            IPropertyBag *pPropertyBag = NULL;

            pMoniker->BindToStorage( NULL,
                                     NULL,
                                     IID_IPropertyBag,
                                     (LPVOID*)&pPropertyBag );

            VARIANT var = {0};

            // get FriendlyName, Description, DevicePath.
            var.vt = VT_BSTR;

            pPropertyBag->Read( L"FriendlyName", &var, 0);
            if ( var.bstrVal != NULL )
            {
                newCDI.name = var.bstrVal;

                if ( newCDIavailed == false )
                    newCDIavailed = true;
            }

            pPropertyBag->Read( L"Description", &var, 0);
            if ( var.bstrVal != NULL )
            {
                newCDI.description = var.bstrVal;

                if ( newCDIavailed == false )
                    newCDIavailed = true;
            }

            pPropertyBag->Read( L"DevicePath", &var, 0);
            if ( var.bstrVal != NULL )
            {
                newCDI.path = var.bstrVal;

                if ( newCDIavailed == false )
                    newCDIavailed = true;
            }

            VariantClear(&var);

            if ( newCDIavailed == true )
            {
                camDevInfo.push_back( newCDI );
            }

            // release
            pMoniker->Release();
            pPropertyBag->Release();
        }

        pCDevEnum->Release();

        if ( retDeviceInfos != NULL )
        {
            retDeviceInfos->assign( camDevInfo.begin(),
                                    camDevInfo.end() );
        }
    }
}

bool DShowCamera::SelectDevice( size_t idx )
{
    return initDevice( idx );
}

bool DShowCamera::SelectConfig( size_t idx )
{
    if( dxdshowprop != NULL )
    {
        if ( ( dxdshowprop->pDeviceFilter != NULL ) &&
             ( dxdshowprop->pCaptureGraphBuilder2 != NULL ) )
        {
            HRESULT hr = S_FALSE;

            if ( dxdshowprop->pConfig == NULL )
            {
                ICaptureGraphBuilder2* pCGB2 = dxdshowprop->pCaptureGraphBuilder2;

                hr = pCGB2->FindInterface( &PIN_CATEGORY_CAPTURE,
                                           NULL,
                                           dxdshowprop->pDeviceFilter,
                                           IID_IAMStreamConfig,
                                           (LPVOID*)&dxdshowprop->pConfig );
                if ( FAILED( hr ) )
                    return false;
            }

            if ( dxdshowprop->pConfig == NULL )
            {
                return false;
            }

            bConfigured = false;
            int iCount = 0;
            int iSize = 0;

            dxdshowprop->pConfig->GetNumberOfCapabilities( &iCount, &iSize );

            if ( iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS) )
            {
                VIDEO_STREAM_CONFIG_CAPS scc;
                AM_MEDIA_TYPE *pmtConfig = NULL;

                hr = dxdshowprop->pConfig->GetStreamCaps( idx,
                                                          &pmtConfig,
                                                          (BYTE*)&scc );

                if ( SUCCEEDED(hr) )
                {
                    hr = dxdshowprop->pConfig->SetFormat( pmtConfig );
                    if ( SUCCEEDED(hr) )
                    {
                        bConfigured = true;
                    }
                }

                _DeleteMediaType( pmtConfig );
            }

            return bConfigured;
        }
    }

    return false;
}

bool DShowCamera::ConnectPIN( bool useVMR9 )
{
    if ( dxdshowprop != NULL )
    {
        if ( dxdshowprop->pConfig != NULL )
        {
            if ( bConfigured == true )
            {
                IGraphBuilder* pGB = dxdshowprop->pGraphBuilder;

                if ( pGB != NULL )
                {
                    pGB->AddFilter( dxdshowprop->pDeviceFilter,
                                    L"Device Filter");

                    ICaptureGraphBuilder2* pCGB2 = dxdshowprop->pCaptureGraphBuilder2;

                    if ( pCGB2 != NULL )
                    {
                        pCGB2->SetFiltergraph( pGB );

                        if ( dxdshowprop->pMediaControl != NULL )
                        {
                            dxdshowprop->pMediaControl->Release();
                        }

                        pGB->QueryInterface( IID_IMediaControl,
                                            (LPVOID*)&dxdshowprop->pMediaControl);

                        if ( useVMR9 == true )
                        {
                            allocVMR9();
                        }

                        return true;
                    }
                }
            }
        }
    }
}

bool DShowCamera::initDevice( size_t idx )
{
    if ( _COMOBJ_INIT == false )
        return false;

    if ( dxdshowprop != NULL )
        return false;

    dxdshowprop = new DxDShowProperties();
    if( dxdshowprop != NULL )
    {
        HRESULT hr = \
        CoCreateInstance( CLSID_FilterGraph,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IGraphBuilder,
                          (LPVOID*)&dxdshowprop->pGraphBuilder );

        if ( FAILED(hr) )
            return false;

        hr = \
        CoCreateInstance( CLSID_CaptureGraphBuilder2,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ICaptureGraphBuilder2,
                          (LPVOID*)&dxdshowprop->pCaptureGraphBuilder2 );

        if ( FAILED(hr) )
        {
            dxdshowprop->pGraphBuilder->Release();
            dxdshowprop->pGraphBuilder = NULL;

            return false;
        }

        enumerateConfigs();

        return true;
    }

    return false;
}

bool DShowCamera::bindDevice( size_t idx )
{
    if ( ( _COMOBJ_INIT == true ) && ( dxdshowprop != NULL ) )
    {
        bool bRet = false;

        ICreateDevEnum* pCDevEnum = NULL;
        IEnumMoniker*   pEnumMoniker = NULL;

        HRESULT hr = \
        CoCreateInstance( CLSID_SystemDeviceEnum,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ICreateDevEnum,
                          (PVOID*)&pCDevEnum );

        if ( SUCCEEDED(hr) && ( pCDevEnum != NULL ) )
        {
            pCDevEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory,
                                              &pEnumMoniker,
                                              0 );
            if (pEnumMoniker == NULL)
            {
                pCDevEnum->Release();
                return false;
            }
        }

        pEnumMoniker->Reset();

        IMoniker* pMoniker = NULL;
        ULONG     nFetched = 0;

        hr = pEnumMoniker->Next( idx, &pMoniker, &nFetched );

        if ( SUCCEEDED(hr) && ( pMoniker != NULL ) )
        {
            hr = \
            pMoniker->BindToObject( NULL,
                                    NULL,
                                    IID_IBaseFilter,
                                    (LPVOID*)&dxdshowprop->pDeviceFilter );

            if ( SUCCEEDED(hr) )
            {
                bRet = true;
            }

            pMoniker->Release();
        }

        pCDevEnum->Release();

        return bRet;
    }

    return false;
}

void DShowCamera::readDeviceSettings()
{
    if ( dxdshowprop != NULL )
    {
        if ( dxdshowprop->pDeviceFilter != NULL )
        {
            resetDeviceSettings();

            IAMCameraControl *pCamCtrl = NULL;
            IBaseFilter* pDF = dxdshowprop->pDeviceFilter;

            HRESULT hr = S_FALSE;

            long Min = 0;
            long Max = 0;
            long Step = 0;
            long Default = 0;
            long Flags = 0;

            hr = pDF->QueryInterface( IID_IAMCameraControl, (LPVOID*)&pCamCtrl );
            if ( SUCCEEDED(hr) )
            {
                hr = pCamCtrl->GetRange( CameraControl_Exposure,
                                         &Min, &Max, &Step, &Default, &Flags );
                if (SUCCEEDED(hr))
                {
                    setExposure.enabled = true;
                    setExposure.minimum_val = Min;
                    setExposure.maximum_val = Max;
                    setExposure.default_val = Default;
                    setExposure.flag = Flags;
                }

                hr = pCamCtrl->GetRange( CameraControl_Focus,
                                         &Min, &Max, &Step, &Default, &Flags );
                if (SUCCEEDED(hr))
                {
                    setFocus.enabled = true;
                    setFocus.minimum_val = Min;
                    setFocus.maximum_val = Max;
                    setFocus.default_val = Default;
                    setFocus.flag = Flags;
                }

                pCamCtrl->Release();
            }

            IAMVideoProcAmp *pProcAmp = NULL;
            hr = pDF->QueryInterface( IID_IAMVideoProcAmp, (LPVOID*)&pProcAmp );
            if ( SUCCEEDED(hr) )
            {
                // Get the range and default values
                hr = pProcAmp->GetRange( VideoProcAmp_Brightness,
                                         &Min, &Max, &Step, &Default, &Flags );
                if ( SUCCEEDED(hr) )
                {
                    setBrightness.enabled = true;
                    setBrightness.minimum_val = Min;
                    setBrightness.maximum_val = Max;
                    setBrightness.default_val = Default;
                    setBrightness.flag = Flags;
                }

                hr = pProcAmp->GetRange( VideoProcAmp_BacklightCompensation,
                                         &Min, &Max, &Step, &Default, &Flags );
                if ( SUCCEEDED(hr) )
                {
                    setBacklightCompensation.enabled = true;
                    setBacklightCompensation.minimum_val = Min;
                    setBacklightCompensation.maximum_val = Max;
                    setBacklightCompensation.default_val = Default;
                    setBacklightCompensation.flag = Flags;
                }

                hr = pProcAmp->GetRange( VideoProcAmp_Contrast,
                                         &Min, &Max, &Step, &Default, &Flags );
                if ( SUCCEEDED(hr) )
                {
                    setContrast.enabled = true;
                    setContrast.minimum_val = Min;
                    setContrast.maximum_val = Max;
                    setContrast.default_val = Default;
                    setContrast.flag = Flags;
                }

                hr = pProcAmp->GetRange( VideoProcAmp_Saturation,
                                         &Min, &Max, &Step, &Default, &Flags);
                if ( SUCCEEDED(hr) )
                {
                    setSaturation.enabled = true;
                    setSaturation.minimum_val = Min;
                    setSaturation.maximum_val = Max;
                    setSaturation.default_val = Default;
                    setSaturation.flag = Flags;
                }

                hr = pProcAmp->GetRange( VideoProcAmp_Sharpness,
                                         &Min, &Max, &Step, &Default, &Flags);
                if ( SUCCEEDED(hr) )
                {
                    setSharpness.enabled = true;
                    setSharpness.minimum_val = Min;
                    setSharpness.maximum_val = Max;
                    setSharpness.default_val = Default;
                    setSharpness.flag = Flags;
                }

                hr = pProcAmp->GetRange( VideoProcAmp_WhiteBalance,
                                         &Min, &Max, &Step, &Default, &Flags);
                if ( SUCCEEDED(hr) )
                {
                    setWhiteBalance.enabled = true;
                    setWhiteBalance.minimum_val = Min;
                    setWhiteBalance.maximum_val = Max;
                    setWhiteBalance.default_val = Default;
                    setWhiteBalance.flag = Flags;
                }

                pProcAmp->Release();
            }
        }
    }
}

void DShowCamera::resetDeviceSettings()
{
    CameraSettingItem* refSPtrs[] = { &setBrightness,
                                      &setContrast,
                                      &setHue,
                                      &setSaturation,
                                      &setSharpness,
                                      &setGamma,
                                      &setColorEnable,
                                      &setWhiteBalance,
                                      &setBacklightCompensation,
                                      &setGain,
                                      &setExposure,
                                      &setFocus,
                                      NULL };

    size_t szrefs = sizeof( refSPtrs );

    for( size_t cnt=0; cnt<szrefs; cnt++ )
    {
        CameraSettingItem* psetting = refSPtrs[ cnt ];
        if ( psetting != NULL )
        {
            memset( psetting, 0, sizeof( CameraSettingItem ) );
        }
    }

}

void DShowCamera::enumerateConfigs()
{
    if( dxdshowprop != NULL )
    {
        if ( dxdshowprop->pDeviceFilter != NULL)
        {
            cfgitems.clear();

            IAMStreamConfig* pConfig = NULL;
            ICaptureGraphBuilder2* pCGB2 = dxdshowprop->pCaptureGraphBuilder2;

            HRESULT hr = \
            pCGB2->FindInterface( &PIN_CATEGORY_CAPTURE,
                                  NULL,
                                  dxdshowprop->pDeviceFilter,
                                  IID_IAMStreamConfig,
                                  (LPVOID*)&pConfig );

            if ( FAILED(hr) )
                return;

            int iCount = 0;
            int iSize = 0;

            pConfig->GetNumberOfCapabilities( &iCount, &iSize );

            if ( iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS) )
            {
                bool bConfigured = false;

                // Use the video capabilities structure.
                for ( int cnt = 0; cnt < iCount; cnt++ )
                {
                    VIDEO_STREAM_CONFIG_CAPS scc;
                    AM_MEDIA_TYPE *pmtCfg = NULL;

                    hr = pConfig->GetStreamCaps( cnt, &pmtCfg, (BYTE*)&scc);
                    if ( SUCCEEDED(hr) )
                    {
                        if ( ( pmtCfg->majortype == MEDIATYPE_Video ) &&
                             ( pmtCfg->cbFormat >= sizeof(VIDEOINFOHEADER) ) &&
                             ( pmtCfg->pbFormat != NULL ) )
                        {
                            VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmtCfg->pbFormat;

                            // Check video header exists.
                            if ( ( pVih != NULL ) &&
                                 ( pmtCfg->formattype == FORMAT_VideoInfo ) )
                            {
                                CameraConfigItem newcfgitem;

                                newcfgitem.encodedtype = DShowCamera::UNSUPPORTED;
                                newcfgitem.width = 0;
                                newcfgitem.height = 0;
                                newcfgitem.bpp = 0;

                                if ( pmtCfg->subtype == MEDIASUBTYPE_YUY2 )
                                    newcfgitem.encodedtype = DShowCamera::YUY2;
                                else
                                if ( pmtCfg->subtype == MEDIASUBTYPE_MJPG )
                                    newcfgitem.encodedtype = DShowCamera::MJPG;
                                else
                                if ( pmtCfg->subtype == MEDIASUBTYPE_RGB565 )
                                    newcfgitem.encodedtype = DShowCamera::RGB565;
                                else
                                if ( pmtCfg->subtype == MEDIASUBTYPE_RGB555 )
                                    newcfgitem.encodedtype = DShowCamera::RGB555;
                                else
                                if ( pmtCfg->subtype == MEDIASUBTYPE_RGB24 )
                                    newcfgitem.encodedtype = DShowCamera::RGB888;

                                newcfgitem.width  = pVih->bmiHeader.biWidth;
                                newcfgitem.height = pVih->bmiHeader.biHeight;
                                newcfgitem.bpp    = pVih->bmiHeader.biBitCount;

                                cfgitems.push_back( newcfgitem );
                            }
                        }

                        _DeleteMediaType( pmtCfg );
                    }
                }
            }
        }
    }
}

void DShowCamera::allocVMR9()
{
    if ( dxdshowprop != NULL )
    {
        IGraphBuilder* pGB = dxdshowprop->pGraphBuilder;

        if ( pGB != NULL )
        {
            if ( dxdshowprop->pVmr == NULL )
            {
                CoCreateInstance( CLSID_VideoMixingRenderer,
                                  NULL,
                                  CLSCTX_INPROC_SERVER ,
                                  IID_IBaseFilter,
                                  (LPVOID*)&dxdshowprop->pVmr );
            }

            if ( dxdshowprop->pVmr != NULL )
            {
                pGB->AddFilter( dxdshowprop->pVmr,
                               L"Video Mixing Renderer 9" );

                IVMRFilterConfig* pVMRConfig = NULL;

                dxdshowprop->pVmr->QueryInterface( IID_IVMRFilterConfig,
                                                  (LPVOID*)&pVMRConfig );
                if ( pVMRConfig != NULL )
                {
                    pVMRConfig->SetRenderingMode( VMRMode_Renderless );
                    pVMRConfig->Release();
                }

                if ( dxdshowprop->pVmrWinCtrl == NULL )
                {
                    dxdshowprop->pVmr->QueryInterface( IID_IVMRWindowlessControl,
                                                      (LPVOID*)&dxdshowprop->pVmrWinCtrl );

                    if ( dxdshowprop->pVmrWinCtrl != NULL )
                    {
                        if ( _hiddenWindow != NULL )
                        {
                            dxdshowprop->pVmrWinCtrl->SetVideoClippingWindow( _hiddenWindow );
                        }
                    }
                }
            }
        }
    }
}
