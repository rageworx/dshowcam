#ifndef __DSHOWCAM_H__
#define __DSHOWCAM_H__
#pragma once

/*******************************************************************************
**
** "DirectShow windows camera control wrapping class" for MinGW-W64 and M$VC
** -----------------------------------------------------------------------------
** (C)Copyright 2017, 2018 Raphael Kim (rageworx@gmail.com, rage.kim@gmail.com)
**
**
*******************************************************************************/

#include <string>
#include <vector>

class DxDShowProperties;

class DShowCamera
{
    public:
        typedef enum
        {
            UNSUPPORTED = -1,
            YUY2 = 0,
            MJPG,
            RGB555,
            RGB565,
            RGB888,
            MAX
        }ENCODE_TYPE;

        typedef struct
        {
            std::wstring name;
            std::wstring description;
            std::wstring path;   // Device Path : "USB\\VID:..."
        }CameraDeviceInfo;

        typedef struct
        {
            ENCODE_TYPE encodedtype;
            unsigned bpp;
            unsigned width;
            unsigned height;
        }CameraConfigItem;

        typedef struct
        {
            bool enabled;
            long minimum_val;
            long maximum_val;
            long default_val;
            long val_step;
            long flag; /// Auto or Manual.
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
        bool   GetConfig( CameraConfigItem &cfg );
        bool   GetConfig( ConfigItems &cfgs );
        bool   SelectDevice( size_t idx );
        bool   SelectConfig( size_t idx );
        bool   ConnectPIN( bool useVMR9 = true );
        bool   SetInteraceFrequncy( size_t idx );

    protected:
        bool initDevice( size_t idx );
        bool bindDevice( size_t idx );
        void readDeviceSettings();
        void resetDeviceSettings();
        void enumerateConfigs();
        void allocVMR9();
        bool doPolling();       /// Enables internal polling capture (thread)

    private:
        DxDShowProperties*  dxdshowprop;
        DeviceInfos         camDevInfo;
        ConfigItems         cfgitems;
        DeviceFreqs         interaceFreqs;
        CameraSettingItem   setBrightness;
        CameraSettingItem   setContrast;
        CameraSettingItem   setHue;
        CameraSettingItem   setSaturation;
        CameraSettingItem   setSharpness;
        CameraSettingItem   setGamma;
        CameraSettingItem   setColorEnable;
        CameraSettingItem   setWhiteBalance;
        CameraSettingItem   setBacklightCompensation;
        CameraSettingItem   setGain;
        CameraSettingItem   setExposure;
        CameraSettingItem   setFocus;
        bool                bConfigured;
};

#endif // __DSHOWCAM_H__
