#include <windows.h>
#include <initguid.h>
#ifdef _MSC_VER 
// Why latest M$VC not supports qedit ? why ???
#include "qeditimp.h"
#else
#include <qedit.h>
#endif /// of _MSC_VER

#include "dshowcam.h"
#include "dshowcamtk.h"
#include "yuvconv.h"

////////////////////////////////////////////////////////////////////////////////

using namespace dshowcamtk;

////////////////////////////////////////////////////////////////////////////////

// Y8 GUID : 20203859-0000-0010-8000-00AA00389B71
#ifndef MEDIASUBTYPE_Y8
DEFINE_GUID(MEDIASUBTYPE_Y8,0x20203859,0x0000,0x0010,\
                            0x80,0x00,0x000,0xAA,0x00,0x38,0x9B,0x71);
#endif // MEDIASUBTYPE_Y8

#ifndef _MSC_VER
// M$VC may not allow to define these, ha !
#ifndef CLSID_SampleGrabber
DEFINE_GUID(CLSID_SampleGrabber,0xc1f400a0,0x3f08,0x11d3, \
                                0x9f,0x0b,0x00,0x60,0x08,0x03,0x9e,0x37);
#endif // CLSID_SampleGrabber

#ifndef CLSID_NullRenderer
DEFINE_GUID(CLSID_NullRenderer,0xc1f400a4,0x3f08,0x11d3,\
                               0x9f,0x0b,0x00,0x60,0x08,0x03,0x9e,0x37);
#endif // CLSID_NullRenderer
#endif /// of _MSC_VER

////////////////////////////////////////////////////////////////////////////////

static bool _COMOBJ_INIT = false;
static int  _COMOBJ_REFCOUNT = 0;

static void _Initialize_DSHOWCOMOBJ();
static void _Finalize_DSHOWCOMOBJ();

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void _Initialize_DSHOWCOMOBJ()
{
    if ( _COMOBJ_INIT == false )
    {
        if ( CoInitialize(NULL) == S_OK )
        {
            _COMOBJ_INIT = true;
        }
    }

    if ( _COMOBJ_REFCOUNT >= 0 )
    {
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

////////////////////////////////////////////////////////////////////////////////

const char* GUID2str(GUID guid)
{
    static char retstr[128] = {0};

    sprintf( retstr,
             "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
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

void PrintMediaType( int idx, AM_MEDIA_TYPE *pmt)
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
            printf( "Error: media major type is not video ! (%d)\n",
                     pmt->majortype );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

class DxDShowProperties
{
    public:
        DxDShowProperties()
         : pGraph( NULL ),
           pCGB( NULL ),
           pControl( NULL ),
           pCameraFilter( NULL ),
           pGrabberFilter( NULL ),
           pSGrabber( NULL ),
           pSourceFilter( NULL ),
           pNullFilter( NULL ),
           bConfigured( false )
        {
            // Initialized.
            memset( &ConfigAMT, 0, sizeof( AM_MEDIA_TYPE ) );
        }

    public:
        ~DxDShowProperties()
        {
            finalizeControl();

            _SafeRelease(pCameraFilter);
            _SafeRelease(pNullFilter);
            _SafeRelease(pSourceFilter);
            _SafeRelease(pSGrabber);
            _SafeRelease(pGrabberFilter);
            _SafeRelease(pControl);
            _SafeRelease(pCGB);
            _SafeRelease(pGraph);
        }

    public:
        ICreateDevEnum*         pSysDevEnum;
        IEnumMoniker*           pEnumCams;
        IMoniker*               pIMon;
        IBaseFilter*            pCameraFilter;

        IGraphBuilder*          pGraph;
        ICaptureGraphBuilder2*  pCGB;
        IMediaControl*          pControl;
        IBaseFilter*            pGrabberFilter;
        ISampleGrabber*         pSGrabber;
        IBaseFilter*            pSourceFilter;
        IBaseFilter*            pNullFilter;

        AM_MEDIA_TYPE           ConfigAMT;
        bool                    bConfigured;

    protected:
        void finalizeControl()
        {
            if ( pControl != NULL )
            {
                OAFilterState cstate = 0;

                pControl->GetState( 100, &cstate );

                if ( cstate != State_Stopped )
                {
                    // Here comes to deadlock ....
                    pControl->Stop();
                }

                _SafeRelease( pControl );
            }

            if ( pSourceFilter != NULL )
            {
                pSourceFilter->Stop();
            }


            if ( pSGrabber != NULL )
            {
                pSGrabber->SetBufferSamples(FALSE);
                pSGrabber->SetOneShot(FALSE);
                pSGrabber->SetCallback(NULL, 1);
            }

            _FreeMediaType( ConfigAMT );
        }
};

////////////////////////////////////////////////////////////////////////////////

// Callback for Grabber.
class SampleGrabberCallback : public ISampleGrabberCB
{
    public:
        SampleGrabberCallback(DShowCamera::ENCODE_TYPE encode_type)
         : doGrabFrame( false ),
           buffLock( false ),
           GrabConvertedBuffer( NULL ),
           GrabConvertedBufferSz( 0 )
        {
            enc_type = encode_type;
            hEventGrab = CreateEvent( NULL, FALSE, FALSE, L"FrameGrabEvent" );
        }

        ~SampleGrabberCallback()
        {
            if ( doGrabFrame == true )
            {
                doGrabFrame = false;
                buffLock = true;
            }

            if ( GrabConvertedBuffer != NULL )
            {
                delete[] GrabConvertedBuffer;
                GrabConvertedBufferSz = 0;
            }

            CloseHandle( hEventGrab );
        }

    public:
        void Size( unsigned w, unsigned h )
        {
            imgWidth = w;
            imgHeight = h;
        }

        void Grab()
        {
            if ( doGrabFrame == false )
            {
                doGrabFrame = true;

                // Now wait for event for a second.
                WaitForSingleObject( hEventGrab, 500 );
            }
        }

        unsigned GetBuffer( BYTE* &pOut )
        {
            if ( ( GrabConvertedBuffer != NULL ) && ( buffLock == false ) )
            {
                if ( GrabConvertedBufferSz > 0 )
                {
                    buffLock = true;

                    pOut = new BYTE[ GrabConvertedBufferSz ];
                    if ( pOut != NULL )
                    {
                        memcpy( pOut,
                                GrabConvertedBuffer,
                                GrabConvertedBufferSz );

                        buffLock = false;
                        return GrabConvertedBufferSz;
                    }

                    buffLock = false;
                }
            }

            return 0;
        }

    public:
        // Fake reference counting.
        STDMETHODIMP_(ULONG) AddRef() { return 1; }
        STDMETHODIMP_(ULONG) Release() { return 2; }
        STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
        {
            if (NULL == ppvObject)
                return E_POINTER;

            if (riid == __uuidof(IUnknown))
            {
                *ppvObject = static_cast<IUnknown*>(this);
                 return S_OK;
            }

			// M$VC not recognizes this type, sucks !
			//if (riid == __uuidof(ISampleGrabberCB))
			if (riid == CLSID_SampleGrabber)
            {
                *ppvObject = static_cast<ISampleGrabberCB*>(this);
                 return S_OK;
            }

            return S_OK;
        }

    public:
        STDMETHODIMP SampleCB(double Time, IMediaSample *pSample)
        {
            return S_OK;
        }

        STDMETHODIMP BufferCB(double Time, BYTE* pBuffer, long BufferLen)
        {
            if ( ( pBuffer == NULL ) || ( BufferLen == 0 ) )
            {
                return E_UNEXPECTED;
            }

            if( ( doGrabFrame == true ) && ( buffLock == false ) )
            {
                doGrabFrame = false;
                buffLock = true;

                if ( GrabConvertedBuffer != NULL )
                {
                    delete[] GrabConvertedBuffer;
                    GrabConvertedBufferSz = 0;
                }

                if ( ( imgWidth > 0 ) && ( imgHeight > 0 ) )
                {
                    switch( enc_type )
                    {
                        case DShowCamera::YUYV:
                            GrabConvertedBufferSz = yuyv2rgb( pBuffer,
                                                              BufferLen,
                                                              (void**)&GrabConvertedBuffer,
                                                              imgWidth,
                                                              imgHeight );
                            break;

                        case DShowCamera::YUVY:
                            GrabConvertedBufferSz = yuyv2rgb( pBuffer,
                                                              BufferLen,
                                                              (void**)&GrabConvertedBuffer,
                                                              imgWidth,
                                                              imgHeight );
                            break;
                    }
                }

                buffLock = false;

                SetEvent( hEventGrab );
            }

            return S_OK;
        }

    protected:
        bool        doGrabFrame;
        bool        buffLock;
        BYTE*       GrabConvertedBuffer;
        unsigned    GrabConvertedBufferSz;
        unsigned    imgWidth;
        unsigned    imgHeight;
        HANDLE      hEventGrab;

    private:
        DShowCamera::ENCODE_TYPE enc_type;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

DShowCamera::DShowCamera()
 : dxdsprop( NULL ),
   bConfigured( false ),
   pSGCB( NULL ),
   currentcfgidx(-1)
{
    _Initialize_DSHOWCOMOBJ();
}

DShowCamera::~DShowCamera()
{
    if ( pSGCB != NULL )
    {
        _SafeRelease( pSGCB );
    }

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
        size_t    idx      = 1;

        while ( pEnumMoniker->Next( 1, &pMoniker, &nFetched ) == S_OK )
        {
            bool newCDIavailed = false;
            CameraDeviceInfo newCDI;

            IPropertyBag *pPropertyBag = NULL;

            pMoniker->BindToStorage( NULL,
                                     NULL,
                                     IID_IPropertyBag,
                                     (LPVOID*)&pPropertyBag );

            newCDI.realindex = idx;

            if( pPropertyBag != NULL )
            {
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
            }

            if ( newCDIavailed == true )
            {
                camDevInfo.push_back( newCDI );
            }

            // release
            _SafeRelease( pMoniker );
            _SafeRelease( pPropertyBag );

            idx++;
        }

        _SafeRelease( pCDevEnum );

        if ( retDeviceInfos != NULL )
        {
            retDeviceInfos->assign( camDevInfo.begin(),
                                    camDevInfo.end() );
        }

        _SafeRelease( pEnumMoniker );
    }
}

size_t DShowCamera::ConfigSize()
{
    return cfgitems.size();
}

bool DShowCamera::GetCurrentConfig( CameraConfigItem &cfg )
{
    if ( bConfigured == true )
    {
        if ( currentcfgidx < cfgitems.size() )
        {
            cfg.encodedtype = cfgitems[currentcfgidx].encodedtype;
            cfg.width       = cfgitems[currentcfgidx].width;
            cfg.height      = cfgitems[currentcfgidx].height;
            cfg.bpp         = cfgitems[currentcfgidx].bpp;
            cfg.realindex   = cfgitems[currentcfgidx].realindex;
        }
    }

    return false;
}

bool DShowCamera::GetConfigs( ConfigItems &cfgs )
{
    if ( cfgitems.size() > 0 )
    {
        cfgs.clear();
        cfgs.assign( cfgitems.begin(), cfgitems.end() );

        return true;
    }

    return false;
}

bool DShowCamera::SelectDevice( size_t idx )
{
    return initDevice( idx );
}

bool DShowCamera::SelectConfig( size_t idx )
{
    if( dxdsprop != NULL )
    {
        if ( ( dxdsprop->pCGB != NULL ) &&
             ( dxdsprop->pCameraFilter != NULL) )
        {
            IAMStreamConfig*       pConfig = NULL;
            ICaptureGraphBuilder2* pCGB2 = dxdsprop->pCGB;

            HRESULT hr = \
            pCGB2->FindInterface( &PIN_CATEGORY_CAPTURE,
                                  NULL,
                                  dxdsprop->pCameraFilter,
                                  IID_IAMStreamConfig,
                                  (LPVOID*)&pConfig );

            if ( FAILED(hr) )
                return false;

            int iCount = 0;
            int iSize = 0;
            size_t realidx = cfgitems[ idx ].realindex;

            pConfig->GetNumberOfCapabilities( &iCount, &iSize );

            if ( ( iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS) ) &&
                 ( realidx < iCount ) )
            {
                VIDEO_STREAM_CONFIG_CAPS scc;
                AM_MEDIA_TYPE *pmtCfg = NULL;

                hr = pConfig->GetStreamCaps( realidx, &pmtCfg, (BYTE*)&scc);
                if ( SUCCEEDED(hr) )
                {
                    if ( dxdsprop->bConfigured == true )
                    {
                        _FreeMediaType( dxdsprop->ConfigAMT );
                    }

                    pConfig->SetFormat( pmtCfg );

                    _CopyMediaType( &dxdsprop->ConfigAMT, pmtCfg );
                    _DeleteMediaType( pmtCfg );

                    if ( pSGCB != NULL )
                    {
                        VIDEOINFOHEADER *pVih = \
                        (VIDEOINFOHEADER*)dxdsprop->ConfigAMT.pbFormat;

                        pSGCB->Size( pVih->bmiHeader.biWidth,
                                     pVih->bmiHeader.biHeight );
                    }

                    currentcfgidx = idx;
                    dxdsprop->bConfigured = true;
                    bConfigured = true;
                }
            }

            _SafeRelease( pConfig );
        }
    }

    return false;
}

bool DShowCamera::GetSetting( SETTING_TYPE settype, CameraSettingItem &item )
{
    if ( settype < SETTING_TYPE_MAX )
    {
        memcpy( &item, &settings[settype], sizeof( CameraSettingItem ) );
        return true;
    }

    return false;
}

bool DShowCamera::ApplyManualSetting( SETTING_TYPE settype, long newVal )
{
    IAMCameraControl* pCamCtrl = NULL;
    IAMVideoProcAmp*  pProcAmp = NULL;

    HRESULT hr = S_FALSE;

    if ( dxdsprop != NULL )
    {
        if ( dxdsprop->pCameraFilter != NULL )
        {
            IBaseFilter* pDF = dxdsprop->pCameraFilter;

            if ( ( settype == EXPOSURE ) || ( settype == FOCUS ) )
            {
                HRESULT hr = S_FALSE;

                hr = pDF->QueryInterface( IID_IAMCameraControl, (LPVOID*)&pCamCtrl );
            }
            else
            {
                hr = pDF->QueryInterface( IID_IAMVideoProcAmp, (LPVOID*)&pProcAmp );
            }
        }
    }

    if ( ( pCamCtrl == NULL ) && ( pProcAmp == NULL ) )
        return false;

    bool retb = false;

    switch( settype )
    {
        case BRIGHTNESS:
            if ( settings[BRIGHTNESS].enabled == true )
            {
                if ( ( settings[BRIGHTNESS].minimum_val <= newVal ) &&
                     ( settings[BRIGHTNESS].maximum_val >= newVal ) )
                {
                    hr = pProcAmp->Set( VideoProcAmp_Brightness,
                                        newVal,
                                        VideoProcAmp_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case CONTRAST:
            if ( settings[CONTRAST].enabled == true )
            {
                if ( ( settings[CONTRAST].minimum_val <= newVal ) &&
                     ( settings[CONTRAST].maximum_val >= newVal ) )
                {
                    hr = pProcAmp->Set( VideoProcAmp_Contrast,
                                        newVal,
                                        VideoProcAmp_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case HUE:
            if ( settings[HUE].enabled == true )
            {
                if ( ( settings[HUE].minimum_val <= newVal ) &&
                     ( settings[HUE].maximum_val >= newVal ) )
                {
                    hr = pProcAmp->Set( VideoProcAmp_Hue,
                                        newVal,
                                        VideoProcAmp_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case SATURATION:
            if ( settings[SATURATION].enabled == true )
            {
                if ( ( settings[SATURATION].minimum_val <= newVal ) &&
                     ( settings[SATURATION].maximum_val >= newVal ) )
                {
                    hr = pProcAmp->Set( VideoProcAmp_Saturation,
                                        newVal,
                                        VideoProcAmp_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case SHARPNESS:
            if ( settings[SHARPNESS].enabled == true )
            {
                if ( ( settings[SHARPNESS].minimum_val <= newVal ) &&
                     ( settings[SHARPNESS].maximum_val >= newVal ) )
                {
                    hr = pProcAmp->Set( VideoProcAmp_Sharpness,
                                        newVal,
                                        VideoProcAmp_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case GAMMA:
            if ( settings[GAMMA].enabled == true )
            {
                if ( ( settings[GAMMA].minimum_val <= newVal ) &&
                     ( settings[GAMMA].maximum_val >= newVal ) )
                {
                    hr = pProcAmp->Set( VideoProcAmp_Gamma,
                                        newVal,
                                        VideoProcAmp_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case COLORENABLE:
            if ( settings[COLORENABLE].enabled == true )
            {
                if ( ( settings[COLORENABLE].minimum_val <= newVal ) &&
                     ( settings[COLORENABLE].maximum_val >= newVal ) )
                {
                    hr = pProcAmp->Set( VideoProcAmp_ColorEnable,
                                        newVal,
                                        VideoProcAmp_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case WHITEBALANCE:
            if ( settings[WHITEBALANCE].enabled == true )
            {
                if ( ( settings[WHITEBALANCE].minimum_val <= newVal ) &&
                     ( settings[WHITEBALANCE].maximum_val >= newVal ) )
                {
                    hr = pProcAmp->Set( VideoProcAmp_WhiteBalance,
                                        newVal,
                                        VideoProcAmp_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case BACKLIGHTCOMPENSATION:
            if ( settings[BACKLIGHTCOMPENSATION].enabled == true )
            {
                if ( ( settings[BACKLIGHTCOMPENSATION].minimum_val <= newVal ) &&
                     ( settings[BACKLIGHTCOMPENSATION].maximum_val >= newVal ) )
                {
                    hr = pProcAmp->Set( VideoProcAmp_BacklightCompensation,
                                        newVal,
                                        VideoProcAmp_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case GAIN:
            if ( settings[GAIN].enabled == true )
            {
                if ( ( settings[GAIN].minimum_val <= newVal ) &&
                     ( settings[GAIN].maximum_val >= newVal ) )
                {
                    hr = pProcAmp->Set( VideoProcAmp_Gain,
                                        newVal,
                                        VideoProcAmp_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case PAN:
            if ( settings[PAN].enabled == true )
            {
                if ( ( settings[PAN].minimum_val <= newVal ) &&
                     ( settings[PAN].maximum_val >= newVal ) )
                {
                    hr = pCamCtrl->Set( CameraControl_Pan,
                                        newVal,
                                        CameraControl_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case TILT:
            if ( settings[TILT].enabled == true )
            {
                if ( ( settings[TILT].minimum_val <= newVal ) &&
                     ( settings[TILT].maximum_val >= newVal ) )
                {
                    hr = pCamCtrl->Set( CameraControl_Tilt,
                                        newVal,
                                        CameraControl_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case ROLL:
            if ( settings[ROLL].enabled == true )
            {
                if ( ( settings[ROLL].minimum_val <= newVal ) &&
                     ( settings[ROLL].maximum_val >= newVal ) )
                {
                    hr = pCamCtrl->Set( CameraControl_Roll,
                                        newVal,
                                        CameraControl_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case ZOOM:
            if ( settings[ZOOM].enabled == true )
            {
                if ( ( settings[ZOOM].minimum_val <= newVal ) &&
                     ( settings[ZOOM].maximum_val >= newVal ) )
                {
                    hr = pCamCtrl->Set( CameraControl_Zoom,
                                        newVal,
                                        CameraControl_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case EXPOSURE:
            if ( settings[EXPOSURE].enabled == true )
            {
                if ( ( settings[EXPOSURE].minimum_val <= newVal ) &&
                     ( settings[EXPOSURE].maximum_val >= newVal ) )
                {
                    hr = pCamCtrl->Set( CameraControl_Exposure,
                                        newVal,
                                        CameraControl_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case IRIS:
            if ( settings[IRIS].enabled == true )
            {
                if ( ( settings[IRIS].minimum_val <= newVal ) &&
                     ( settings[IRIS].maximum_val >= newVal ) )
                {
                    hr = pCamCtrl->Set( CameraControl_Iris,
                                        newVal,
                                        CameraControl_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case FOCUS:
            if ( settings[FOCUS].enabled == true )
            {
                if ( ( settings[FOCUS].minimum_val <= newVal ) &&
                     ( settings[FOCUS].maximum_val >= newVal ) )
                {
                    hr = pCamCtrl->Set( CameraControl_Focus,
                                        newVal,
                                        CameraControl_Flags_Manual );
                }

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;
    }

    _SafeRelease( pCamCtrl );
    _SafeRelease( pProcAmp );

    return retb;
}

bool DShowCamera::ApplyAutoSetting( SETTING_TYPE settype )
{
    IAMCameraControl* pCamCtrl = NULL;
    IAMVideoProcAmp*  pProcAmp = NULL;

    HRESULT hr = S_FALSE;

    if ( dxdsprop != NULL )
    {
        if ( dxdsprop->pCameraFilter != NULL )
        {
            IBaseFilter* pDF = dxdsprop->pCameraFilter;

            if ( ( settype == EXPOSURE ) || ( settype == FOCUS ) )
            {
                HRESULT hr = S_FALSE;

                hr = pDF->QueryInterface( IID_IAMCameraControl, (LPVOID*)&pCamCtrl );
            }
            else
            {
                hr = pDF->QueryInterface( IID_IAMVideoProcAmp, (LPVOID*)&pProcAmp );
            }
        }
    }

    if ( ( pCamCtrl == NULL ) && ( pProcAmp == NULL ) )
        return false;

    bool retb = false;

    switch( settype )
    {
        case BRIGHTNESS:
            if ( settings[BRIGHTNESS].enabled == true )
            {
                hr = pProcAmp->Set( VideoProcAmp_Brightness,
                                    settings[BRIGHTNESS].default_val,
                                    VideoProcAmp_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case CONTRAST:
            if ( settings[CONTRAST].enabled == true )
            {
                hr = pProcAmp->Set( VideoProcAmp_Contrast,
                                    settings[CONTRAST].default_val,
                                    VideoProcAmp_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case HUE:
            if ( settings[HUE].enabled == true )
            {
                hr = pProcAmp->Set( VideoProcAmp_Hue,
                                    settings[HUE].default_val,
                                    VideoProcAmp_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case SATURATION:
            if ( settings[SATURATION].enabled == true )
            {
                hr = pProcAmp->Set( VideoProcAmp_Saturation,
                                    settings[SATURATION].default_val,
                                    VideoProcAmp_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case SHARPNESS:
            if ( settings[SHARPNESS].enabled == true )
            {
                hr = pProcAmp->Set( VideoProcAmp_Sharpness,
                                    settings[SHARPNESS].default_val,
                                    VideoProcAmp_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case GAMMA:
            if ( settings[GAMMA].enabled == true )
            {
                hr = pProcAmp->Set( VideoProcAmp_Gamma,
                                    settings[GAMMA].default_val,
                                    VideoProcAmp_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case COLORENABLE:
            if ( settings[COLORENABLE].enabled == true )
            {
                hr = pProcAmp->Set( VideoProcAmp_ColorEnable,
                                    settings[COLORENABLE].default_val,
                                    VideoProcAmp_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case WHITEBALANCE:
            if ( settings[WHITEBALANCE].enabled == true )
            {
                hr = pProcAmp->Set( VideoProcAmp_WhiteBalance,
                                    settings[WHITEBALANCE].default_val,
                                    VideoProcAmp_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case BACKLIGHTCOMPENSATION:
            if ( settings[BACKLIGHTCOMPENSATION].enabled == true )
            {
                hr = pProcAmp->Set( VideoProcAmp_BacklightCompensation,
                                    settings[BACKLIGHTCOMPENSATION].default_val,
                                    VideoProcAmp_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case GAIN:
            if ( settings[GAIN].enabled == true )
            {
                hr = pProcAmp->Set( VideoProcAmp_Gain,
                                    settings[GAIN].default_val,
                                    VideoProcAmp_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case PAN:
            if ( settings[PAN].enabled == true )
            {
                hr = pCamCtrl->Set( CameraControl_Pan,
                                    settings[PAN].default_val,
                                    CameraControl_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case TILT:
            if ( settings[TILT].enabled == true )
            {
                hr = pCamCtrl->Set( CameraControl_Tilt,
                                    settings[TILT].default_val,
                                    CameraControl_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case ROLL:
            if ( settings[ROLL].enabled == true )
            {
                hr = pCamCtrl->Set( CameraControl_Roll,
                                    settings[ROLL].default_val,
                                    CameraControl_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case ZOOM:
            if ( settings[ZOOM].enabled == true )
            {
                hr = pCamCtrl->Set( CameraControl_Zoom,
                                    settings[ZOOM].default_val,
                                    CameraControl_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case EXPOSURE:
            if ( settings[EXPOSURE].enabled == true )
            {
                hr = pCamCtrl->Set( CameraControl_Exposure,
                                    settings[EXPOSURE].default_val,
                                    CameraControl_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case IRIS:
            if ( settings[IRIS].enabled == true )
            {
                hr = pCamCtrl->Set( CameraControl_Iris,
                                    settings[IRIS].default_val,
                                    CameraControl_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;

        case FOCUS:
            if ( settings[FOCUS].enabled == true )
            {
                hr = pCamCtrl->Set( CameraControl_Focus,
                                    settings[FOCUS].default_val,
                                    CameraControl_Flags_Auto );

                if ( SUCCEEDED(hr) )
                {
                    retb = true;
                }
            }
            break;
    }

    _SafeRelease( pCamCtrl );
    _SafeRelease( pProcAmp );

    return retb;
}

bool DShowCamera::StartPoll()
{
    if ( dxdsprop != NULL )
    {
        if ( dxdsprop->pControl != NULL )
        {
            OAFilterState cstate = 0;

            dxdsprop->pControl->GetState( 100, &cstate );

            if ( cstate != State_Running )
            {
                dxdsprop->pControl->Run();
            }

            return true;
        }
    }

    return false;
}

bool DShowCamera::StopPoll()
{
    if ( dxdsprop != NULL )
    {
        if ( dxdsprop->pSourceFilter != NULL )
        {
            dxdsprop->pSourceFilter->Stop();
        }

        if ( dxdsprop->pControl != NULL )
        {
            OAFilterState cstate = 0;

            dxdsprop->pControl->GetState( 100, &cstate );

            if ( cstate != State_Stopped )
            {
                dxdsprop->pControl->Stop();
            }

            return true;
        }
    }

    return false;
}

bool DShowCamera::GrabAFrame( unsigned char* &buff, unsigned &bufflen )
{
    if ( dxdsprop != NULL )
    {
        if ( dxdsprop->pControl != NULL )
        {
            OAFilterState cstate = 0;

            dxdsprop->pControl->GetState( 100, &cstate );

            if ( cstate == State_Running )
            {
                pSGCB->Grab();
                BYTE* tmpbuff = NULL;
                bufflen = pSGCB->GetBuffer( buff );

                return true;
            }
        }
    }

    return false;
}


bool DShowCamera::initDevice( size_t idx )
{
    if ( _COMOBJ_INIT == false )
        return false;

    if ( dxdsprop != NULL )
        return false;

    dxdsprop = new DxDShowProperties();
    if( dxdsprop != NULL )
    {
        bool retb = false;

        if ( camDevInfo.size() > 0 )
        {
            retb = connectDevice( idx );

            if ( retb == true )
            {
                enumerateConfigs();
                retb = configureDevice();
            }
        }

        return retb;
    }

    return false;
}

bool DShowCamera::connectDevice( size_t idx )
{
    if ( dxdsprop != NULL )
    {
        if ( idx >= camDevInfo.size() )
            return false;

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

            if ( pEnumMoniker == NULL )
            {
                _SafeRelease( pCDevEnum );
                return false;
            }
        }

        pEnumMoniker->Reset();

        IMoniker* pMoniker = NULL;
        ULONG     nIdx     = camDevInfo[idx].realindex;
        bool      retb     = false;

        for( ULONG cnt=0;cnt<nIdx; cnt++)
        {
            hr = pEnumMoniker->Next( 1, &pMoniker, NULL );
        }

        if ( pMoniker != NULL )
        {
            if( dxdsprop->pCameraFilter == NULL )
            {
                pMoniker->BindToObject( NULL, NULL, IID_IBaseFilter,
                                        (LPVOID*)&dxdsprop->pCameraFilter );

                if ( dxdsprop->pCameraFilter != NULL )
                {
                    retb = true;
                }
            }
        }

        _SafeRelease( pEnumMoniker );
        _SafeRelease( pCDevEnum );

        return retb;
    }

    return false;
}

bool DShowCamera::configureDevice()
{
    if ( dxdsprop != NULL )
    {
        if ( dxdsprop->pCameraFilter != NULL )
        {
            if ( dxdsprop->pGraph == NULL )
            {
                CoCreateInstance( CLSID_FilterGraphNoThread,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&dxdsprop->pGraph) );
            }

            if ( dxdsprop->pGraph != NULL )
            {
                if ( dxdsprop->pControl == NULL )
                {
                    dxdsprop->\
                    pGraph->QueryInterface( IID_PPV_ARGS(&dxdsprop->pControl) );
                }

                dxdsprop->pGraph->AddFilter( dxdsprop->pCameraFilter,
                                                L"Capture Source" );

                if ( dxdsprop->pGrabberFilter == NULL )
                {
                    CoCreateInstance( CLSID_SampleGrabber,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&dxdsprop->pGrabberFilter) );
                }

                if ( dxdsprop->pGrabberFilter != NULL )
                {
                    dxdsprop->pGraph->AddFilter( dxdsprop->pGrabberFilter,
                                                    L"Sample Grabber" );

                    if ( dxdsprop->pSGrabber == NULL )
                    {
                        dxdsprop->\
                        pGrabberFilter->\
                        QueryInterface( IID_ISampleGrabber, (LPVOID*)&dxdsprop->pSGrabber );
                    }
                }

                if ( dxdsprop->pCGB == NULL )
                {
                    CoCreateInstance( CLSID_CaptureGraphBuilder2,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_ICaptureGraphBuilder2,
                                      (LPVOID *)&dxdsprop->pCGB );
                }

                IEnumPins* pEnum = NULL;

                if ( dxdsprop->pCameraFilter->EnumPins( &pEnum) == S_OK )
                {
                    IPin* pPin = NULL;

                    while ( S_OK == pEnum->Next(1, &pPin, NULL) )
                    {
                        HRESULT hr = \
                        ConnectFilters( dxdsprop->pGraph,
                                        pPin,
                                        dxdsprop->pGrabberFilter );
                        _SafeRelease(pPin);

                        if ( SUCCEEDED(hr) )
                        {
                            break;
                        }
                    }
                }

                if ( dxdsprop->pNullFilter == NULL )
                {
                    CoCreateInstance( CLSID_NullRenderer,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS( &dxdsprop->pNullFilter) );
                }

                if ( dxdsprop->pNullFilter != NULL )
                {
                    dxdsprop->pGraph->AddFilter( dxdsprop->pNullFilter,
                                                    L"NULL Renderer Filter" );
                    ConnectFilters( dxdsprop->pGraph,
                                    dxdsprop->pGrabberFilter,
                                    dxdsprop->pNullFilter );
                }
            }

            if ( dxdsprop->pSGrabber != NULL )
            {
                if ( pSGCB == NULL )
                {
                    pSGCB = new SampleGrabberCallback( DShowCamera::YUYV );
                }

                dxdsprop->pSGrabber->SetOneShot( FALSE );
                dxdsprop->pSGrabber->SetCallback( pSGCB, 1 );
                dxdsprop->pSGrabber->SetBufferSamples( TRUE );
            }

            readDeviceSettings();

            return true;
        }
    }

    return false;
}

void DShowCamera::readDeviceSettings()
{
    if ( dxdsprop != NULL )
    {
        if ( dxdsprop->pCameraFilter != NULL )
        {
            resetDeviceSettings();

            IAMCameraControl *pCamCtrl = NULL;
            IBaseFilter* pDF = dxdsprop->pCameraFilter;

            HRESULT hr = S_FALSE;

            long Min = 0;
            long Max = 0;
            long Step = 0;
            long Default = 0;
            long Flags = 0;

            hr = pDF->QueryInterface( IID_IAMCameraControl, (LPVOID*)&pCamCtrl );
            if ( SUCCEEDED(hr) )
            {
                hr = pCamCtrl->GetRange( CameraControl_Pan,
                                         &Min, &Max, &Step, &Default, &Flags );
                if (SUCCEEDED(hr))
                {
                    settings[PAN].enabled = true;
                    settings[PAN].minimum_val = Min;
                    settings[PAN].maximum_val = Max;
                    settings[PAN].default_val = Default;
                    settings[PAN].flag = Flags;
                }

                hr = pCamCtrl->GetRange( CameraControl_Tilt,
                                         &Min, &Max, &Step, &Default, &Flags );
                if (SUCCEEDED(hr))
                {
                    settings[TILT].enabled = true;
                    settings[TILT].minimum_val = Min;
                    settings[TILT].maximum_val = Max;
                    settings[TILT].default_val = Default;
                    settings[TILT].flag = Flags;
                }

                hr = pCamCtrl->GetRange( CameraControl_Roll,
                                         &Min, &Max, &Step, &Default, &Flags );
                if (SUCCEEDED(hr))
                {
                    settings[ROLL].enabled = true;
                    settings[ROLL].minimum_val = Min;
                    settings[ROLL].maximum_val = Max;
                    settings[ROLL].default_val = Default;
                    settings[ROLL].flag = Flags;
                }

                hr = pCamCtrl->GetRange( CameraControl_Zoom,
                                         &Min, &Max, &Step, &Default, &Flags );
                if (SUCCEEDED(hr))
                {
                    settings[ZOOM].enabled = true;
                    settings[ZOOM].minimum_val = Min;
                    settings[ZOOM].maximum_val = Max;
                    settings[ZOOM].default_val = Default;
                    settings[ZOOM].flag = Flags;
                }

                hr = pCamCtrl->GetRange( CameraControl_Exposure,
                                         &Min, &Max, &Step, &Default, &Flags );
                if (SUCCEEDED(hr))
                {
                    settings[EXPOSURE].enabled = true;
                    settings[EXPOSURE].minimum_val = Min;
                    settings[EXPOSURE].maximum_val = Max;
                    settings[EXPOSURE].default_val = Default;
                    settings[EXPOSURE].flag = Flags;
                }

                hr = pCamCtrl->GetRange( CameraControl_Iris,
                                         &Min, &Max, &Step, &Default, &Flags );
                if (SUCCEEDED(hr))
                {
                    settings[IRIS].enabled = true;
                    settings[IRIS].minimum_val = Min;
                    settings[IRIS].maximum_val = Max;
                    settings[IRIS].default_val = Default;
                    settings[IRIS].flag = Flags;
                }

                hr = pCamCtrl->GetRange( CameraControl_Focus,
                                         &Min, &Max, &Step, &Default, &Flags );
                if (SUCCEEDED(hr))
                {
                    settings[FOCUS].enabled = true;
                    settings[FOCUS].minimum_val = Min;
                    settings[FOCUS].maximum_val = Max;
                    settings[FOCUS].default_val = Default;
                    settings[FOCUS].flag = Flags;
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
                    settings[BRIGHTNESS].enabled = true;
                    settings[BRIGHTNESS].minimum_val = Min;
                    settings[BRIGHTNESS].maximum_val = Max;
                    settings[BRIGHTNESS].default_val = Default;
                    settings[BRIGHTNESS].flag = Flags;
                }

                hr = pProcAmp->GetRange( VideoProcAmp_BacklightCompensation,
                                         &Min, &Max, &Step, &Default, &Flags );
                if ( SUCCEEDED(hr) )
                {
                    settings[BACKLIGHTCOMPENSATION].enabled = true;
                    settings[BACKLIGHTCOMPENSATION].minimum_val = Min;
                    settings[BACKLIGHTCOMPENSATION].maximum_val = Max;
                    settings[BACKLIGHTCOMPENSATION].default_val = Default;
                    settings[BACKLIGHTCOMPENSATION].flag = Flags;
                }

                hr = pProcAmp->GetRange( VideoProcAmp_Contrast,
                                         &Min, &Max, &Step, &Default, &Flags );
                if ( SUCCEEDED(hr) )
                {
                    settings[CONTRAST].enabled = true;
                    settings[CONTRAST].minimum_val = Min;
                    settings[CONTRAST].maximum_val = Max;
                    settings[CONTRAST].default_val = Default;
                    settings[CONTRAST].flag = Flags;
                }

                hr = pProcAmp->GetRange( VideoProcAmp_Saturation,
                                         &Min, &Max, &Step, &Default, &Flags);
                if ( SUCCEEDED(hr) )
                {
                    settings[SATURATION].enabled = true;
                    settings[SATURATION].minimum_val = Min;
                    settings[SATURATION].maximum_val = Max;
                    settings[SATURATION].default_val = Default;
                    settings[SATURATION].flag = Flags;
                }

                hr = pProcAmp->GetRange( VideoProcAmp_Sharpness,
                                         &Min, &Max, &Step, &Default, &Flags);
                if ( SUCCEEDED(hr) )
                {
                    settings[SHARPNESS].enabled = true;
                    settings[SHARPNESS].minimum_val = Min;
                    settings[SHARPNESS].maximum_val = Max;
                    settings[SHARPNESS].default_val = Default;
                    settings[SHARPNESS].flag = Flags;
                }

                hr = pProcAmp->GetRange( VideoProcAmp_WhiteBalance,
                                         &Min, &Max, &Step, &Default, &Flags);
                if ( SUCCEEDED(hr) )
                {
                    settings[WHITEBALANCE].enabled = true;
                    settings[WHITEBALANCE].minimum_val = Min;
                    settings[WHITEBALANCE].maximum_val = Max;
                    settings[WHITEBALANCE].default_val = Default;
                    settings[WHITEBALANCE].flag = Flags;
                }

                pProcAmp->Release();
            }
        }
    }
}

void DShowCamera::resetDeviceSettings()
{
    for( size_t cnt=0; cnt<SETTING_TYPE_MAX; cnt++ )
    {
        memset( &settings[cnt], 0, sizeof( CameraSettingItem ) );
    }
}

void DShowCamera::enumerateConfigs()
{
    if( dxdsprop != NULL )
    {
        if ( dxdsprop->pCGB == NULL )
        {
            CoCreateInstance( CLSID_CaptureGraphBuilder2,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_ICaptureGraphBuilder2,
                              (LPVOID *)&dxdsprop->pCGB );
        }

        if ( ( dxdsprop->pCGB != NULL ) &&
             ( dxdsprop->pCameraFilter != NULL) )
        {
            cfgitems.clear();

            IAMStreamConfig*       pConfig = NULL;
            ICaptureGraphBuilder2* pCGB2 = dxdsprop->pCGB;

            HRESULT hr = \
            pCGB2->FindInterface( &PIN_CATEGORY_CAPTURE,
                                  NULL,
                                  dxdsprop->pCameraFilter,
                                  IID_IAMStreamConfig,
                                  (LPVOID*)&pConfig );

            if ( FAILED(hr) )
                return;

            int iCount = 0;
            int iSize = 0;

            pConfig->GetNumberOfCapabilities( &iCount, &iSize );

            if ( iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS) )
            {
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
                                newcfgitem.realindex = cnt;

                                if ( pmtCfg->subtype == MEDIASUBTYPE_YUY2 )
                                    newcfgitem.encodedtype = DShowCamera::YUYV;
                                else
                                if ( pmtCfg->subtype == MEDIASUBTYPE_YUYV )
                                    newcfgitem.encodedtype = DShowCamera::YUYV;

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

            _SafeRelease( pConfig );
        }
    }
}
