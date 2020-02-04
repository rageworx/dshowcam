#ifndef __DSHOWCAM_H__
#define __DSHOWCAM_H__
#pragma once

/*******************************************************************************
**
** "DirectShow windows YUVx USB camera control wrapping class"
**                                                       for MinGW-W64 and M$VC
**
** -----------------------------------------------------------------------------
** (C)Copyright 2017, 2018 Raphael Kim (rageworx@gmail.com, rage.kim@gmail.com)
**
** Made for : Continuous grabbing frame target ( Movie, or still )
**
*******************************************************************************/

#include <string>
#include <vector>

class DxDShowProperties;
class SampleGrabberCallback;

class DShowCamera
{
    public:
        typedef enum
        {
            UNSUPPORTED = -1,
            YUYV = 0,
            YUVY,
            RGB555,
            RGB565,
        #ifndef _MSC_VER
            MJPEG,
        #endif
            BYPASS,
            ENCODE_TYPE_MAX
        }ENCODE_TYPE;

        typedef enum
        {
            BRIGHTNESS = 0,
            CONTRAST,
            HUE,
            SATURATION,
            SHARPNESS,
            GAMMA,
            COLORENABLE,
            WHITEBALANCE,
            BACKLIGHTCOMPENSATION,
            GAIN,

            PAN,
            TILT,
            ROLL,
            ZOOM,
            EXPOSURE,
            IRIS,
            FOCUS,
            SETTING_TYPE_MAX
        }SETTING_TYPE;

        typedef struct
        {
            std::wstring name;
            std::wstring description;
            std::wstring path;   /// Device Path : "USB\\VID:..."
            size_t realindex;
        }CameraDeviceInfo;

        typedef struct
        {
            ENCODE_TYPE encodedtype;
            unsigned bpp;
            unsigned width;
            unsigned height;
            size_t   realindex;
        }CameraConfigItem;

        typedef struct
        {
            bool enabled;
            long minimum_val;
            long maximum_val;
            long default_val;
            long val_step;
            long flag;
        }CameraSettingItem;

        typedef std::vector< CameraDeviceInfo > DeviceInfos;
        typedef std::vector< CameraConfigItem > ConfigItems;
        typedef std::vector< unsigned long >    DeviceFreqs;

    public:
        DShowCamera();
        virtual ~DShowCamera();

    public:
        void EnermateDevice( DeviceInfos* retDeviceInfos );
        bool IsConfigured() { return bConfigured; }

    public:
        size_t ConfigSize();
        bool   GetCurrentConfig( CameraConfigItem &cfg );
        bool   GetConfigs( ConfigItems &cfgs );
        bool   SelectDevice( size_t idx );
        bool   SelectConfig( size_t idx );
        bool   GetSetting( SETTING_TYPE settype, CameraSettingItem &item );
        bool   ApplyManualSetting( SETTING_TYPE settype, long newVal );
        bool   ApplyAutoSetting( SETTING_TYPE settype );
        bool   StartPoll();
        bool   StopPoll();
        bool   SetGrabRaw( bool onoff );
        bool   GrabAFrame( unsigned char* &buff, unsigned &bufflen );
        bool   GrabTriggered( unsigned char* &buff, unsigned &bufflen );

    protected:
        bool initDevice( size_t idx );
        bool connectDevice( size_t idx );
        bool configureDevice();
        void readDeviceSettings();
        void resetDeviceSettings();
        void enumerateConfigs();

    private:
        DxDShowProperties*  dxdsprop;
        DeviceInfos         camDevInfo;
        ConfigItems         cfgitems;
        CameraSettingItem   settings[SETTING_TYPE_MAX];
        bool                bConfigured;
        bool                bPolling;
        int                 currentcfgidx;

    protected:
        SampleGrabberCallback*  pSGCB;
};

#endif // __DSHOWCAM_H__
