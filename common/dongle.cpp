//    Copyright 2013 (c) AMSAT-UK
//
//    This file is part of FUNcubeLib.
//
//    FUNcubeLib is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    FUNcubeLib is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with FUNcubeLib If not, see <http://www.gnu.org/licenses/>.
//

/// @file Dongle.cpp
//
//////////////////////////////////////////////////////////////////////
#include "FuncubeLib.h"

#ifdef WIN32
#include <setupapi.h>
#include <tchar.h>
#else
#include "fcd.h"
#include "fcdhidcmd.h"
#endif

#include "Dongle.h"


#ifdef WIN32
#define FCD_HID_CMD_QUERY              1 // Returns string with "FCDAPP version"

#define FCD_HID_CMD_SET_FREQUENCY    100 // Send with 3 byte unsigned little endian frequency in kHz.
#define FCD_HID_CMD_SET_FREQUENCY_HZ 101 // Send with 4 byte unsigned little endian frequency in Hz, returns wit actual frequency set in Hz
#define FCD_HID_CMD_GET_FREQUENCY_HZ 102 // Returns 4 byte unsigned little endian frequency in Hz.

#define FCD_HID_CMD_SET_DC_CORR      106 // Send with 2 byte unsigned I DC correction followed by 2 byte unsigned Q DC correction. 32768 is the default centre value.
#define FCD_HID_CMD_GET_DC_CORR      107 // Returns 2 byte unsigned I DC correction followed by 2 byte unsigned Q DC correction. 32768 is the default centre value.
#define FCD_HID_CMD_SET_IQ_CORR      108 // Send with 2 byte signed phase correction followed by 2 byte unsigned gain correction. 0 is the default centre value for phase correction, 32768 is the default centre value for gain.
#define FCD_HID_CMD_GET_IQ_CORR      109 // Returns 2 byte signed phase correction followed by 2 byte unsigned gain correction. 0 is the default centre value for phase correction, 32768 is the default centre value for gain.

#define FCD_HID_CMD_SET_BIAS_CURRENT 126
#define FCD_HID_CMD_GET_BIAS_CURRENT 127

#define FCD_RESET                    255 // Reset to bootloader
//                            
#define FCD_DEVICE_ID_COUNT 2
static TCHAR* m_szVIDPID[]={ 
    _T("Vid_04d8&Pid_fb56"), // pro
    _T("vid_04d8&Pid_fb31"), // pro+
}; 

// CDongle

BOOL CDongle::HIDOpen(PTCHAR pszDevSearchID, PTCHAR pszDevID, PHANDLE phWrite, PHANDLE phRead)
{
    GUID InterfaceClassGuid = {0x4d1e55b2, 0xf16f, 0x11cf, 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30};     
    
    DWORD i;
    PTCHAR pszDevSearchIDLower=_tcsdup(pszDevSearchID);

    _tcslwr(pszDevSearchIDLower);

    // Create a HDEVINFO with all present devices.
    HDEVINFO hDevInfo = SetupDiGetClassDevs
    (
        &InterfaceClassGuid,
        0, // Enumerator
        0,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE 
    );
   
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        // Insert error handling here.
        return FALSE;
    }
   
    // Enumerate through all devices in Set.
    SP_DEVINFO_DATA deviceInfoData = { sizeof(SP_DEVINFO_DATA) };
    DWORD dataType;
    LPTSTR buffer = NULL;
    DWORD bufferSize = 0;
    for (i=0;SetupDiEnumDeviceInfo(hDevInfo,i,&deviceInfoData);i++)
    {        
        // second time round buffer may be big enough
        if(!SetupDiGetDeviceRegistryProperty(hDevInfo, &deviceInfoData, SPDRP_HARDWAREID, &dataType, (PBYTE)buffer, bufferSize, &bufferSize))
        {        
            LocalFree(buffer);
            buffer = (LPTSTR)LocalAlloc(LPTR, bufferSize * sizeof(TCHAR));
            SetupDiGetDeviceRegistryProperty(hDevInfo, &deviceInfoData, SPDRP_HARDWAREID, &dataType, (PBYTE)buffer, bufferSize, NULL);
        }
        _tcslwr(buffer);
       
        if (_tcsstr(buffer,pszDevSearchIDLower)!=NULL)
        {
            // We have found the device, now lets try to open a handles to it
            SP_DEVICE_INTERFACE_DATA dids;
            dids.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
            if(SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &InterfaceClassGuid, i, &dids))
            {
                DWORD requiredSize=0;
                SetupDiGetDeviceInterfaceDetail(hDevInfo,&dids,NULL,0,&requiredSize,NULL);
                PSP_DEVICE_INTERFACE_DETAIL_DATA pdidds=(PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);

                pdidds->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                SetupDiGetDeviceInterfaceDetail(hDevInfo,&dids,pdidds,requiredSize,NULL,NULL);

                if (INVALID_HANDLE_VALUE!=(*phWrite=CreateFile(pdidds->DevicePath,GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,0)))
                {
                    if (INVALID_HANDLE_VALUE!=(*phRead=CreateFile(pdidds->DevicePath,GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,0)))
                    {
                        SetupDiDestroyDeviceInfoList(hDevInfo);
                        free(pszDevSearchIDLower);
                        _tcscpy(pszDevID,buffer);
                        LocalFree(buffer);
                        return TRUE;
                    }
                }
            }            
        }
    }
    LocalFree(buffer);

    if ( GetLastError()!=NO_ERROR && GetLastError()!=ERROR_NO_MORE_ITEMS )
    {
       return FALSE;
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    free(pszDevSearchIDLower);
    return FALSE;
}

void CDongle::HIDClose(HANDLE hWrite,HANDLE hRead)
{
    CloseHandle(hWrite);
    CloseHandle(hRead);
}

unsigned char CDongle::HIDSetFreq(HANDLE hWrite, HANDLE hRead,int nFreq)
{
    if(NULL==hWrite|| INVALID_HANDLE_VALUE==hWrite || NULL==hRead || INVALID_HANDLE_VALUE==hRead)
    {
        return NULL;
    }
    DWORD dwBytesWritten=0;
    DWORD dwBytesRead=0;
    unsigned char au8BufOut[65]; // endpoint size + 1
    unsigned char au8BufIn[65]; // endpoint size + 1

    au8BufOut[0]=0; // First byte is report ID. Ignored by HID Class firmware as only config'd for one report
    au8BufOut[1]=FCD_HID_CMD_SET_FREQUENCY_HZ;
    au8BufOut[2]=(unsigned char)nFreq;
    au8BufOut[3]=(unsigned char)(nFreq>>8);
    au8BufOut[4]=(unsigned char)(nFreq>>16);
    au8BufOut[5]=(unsigned char)(nFreq>>24);
    WriteFile(hWrite,au8BufOut,65,&dwBytesWritten,0);
    Sleep(10);
    ReadFile(hRead,au8BufIn,65,&dwBytesRead,0);
    return au8BufIn[2];
}

ULONG CDongle::HIDGetFreq(HANDLE hWrite, HANDLE hRead)
{
    if(NULL==hWrite|| INVALID_HANDLE_VALUE==hWrite || NULL==hRead || INVALID_HANDLE_VALUE==hRead)
    {
        return NULL;
    }
    DWORD dwBytesWritten=0;
    DWORD dwBytesRead=0;
    unsigned char au8BufOut[65]; // endpoint size + 1
    unsigned char au8BufIn[65]; // endpoint size + 1
    
    au8BufOut[0]=0; // First byte is report ID. Ignored by HID Class firmware as only config'd for one report
    au8BufOut[1]=FCD_HID_CMD_GET_FREQUENCY_HZ;

    WriteFile(hWrite,au8BufOut,65,&dwBytesWritten,0);
    Sleep(10);
    ReadFile(hRead,au8BufIn,65,&dwBytesRead,0);
    
    ULONG freq=0;
    if(1==au8BufIn[2])
    {
        freq=au8BufIn[3];
        freq|=au8BufIn[4]<<8;
        freq|=au8BufIn[5]<<16;
        freq|=au8BufIn[6]<<24;
    }    
    return freq;
}

unsigned char CDongle::HIDResetFCD(HANDLE hWrite, HANDLE hRead)
{
    if(NULL==hWrite|| INVALID_HANDLE_VALUE==hWrite || NULL==hRead || INVALID_HANDLE_VALUE==hRead)
    {
        return NULL;
    }
    DWORD dwBytesWritten=0;
    DWORD dwBytesRead=0;
    unsigned char au8BufOut[65]; // endpoint size + 1
    unsigned char au8BufIn[65]; // endpoint size + 1

    au8BufOut[0]=0; // First byte is report ID. Ignored by HID Class firmware as only config'd for one report
    au8BufOut[1]=FCD_RESET;
    WriteFile(hWrite,au8BufOut,65,&dwBytesWritten,0);
    Sleep(10);
    ReadFile(hRead,au8BufIn,65,&dwBytesRead,0);
    return au8BufIn[2];
}

unsigned char CDongle::HIDSetBiasT(HANDLE hWrite, HANDLE hRead, unsigned __int8 u8Val)
{
    if(NULL==hWrite|| INVALID_HANDLE_VALUE==hWrite || NULL==hRead || INVALID_HANDLE_VALUE==hRead)
    {
        return NULL;
    }
    DWORD dwBytesWritten=0;
    DWORD dwBytesRead=0;
    unsigned char au8BufOut[65]; // endpoint size + 1
    unsigned char au8BufIn[65]; // endpoint size + 1
    memset(au8BufOut, 0xcc, sizeof(au8BufOut));

    au8BufOut[0]=0; // First byte is report ID. Ignored by HID Class firmware as only config'd for one report
    au8BufOut[1]=FCD_HID_CMD_SET_BIAS_CURRENT;
    au8BufOut[2]=u8Val;    
    WriteFile(hWrite,au8BufOut,65,&dwBytesWritten,0);
    Sleep(30);
    ReadFile(hRead,au8BufIn,65,&dwBytesRead,0);
    return au8BufIn[2];
}

BOOL CDongle::HIDReadCommand(HANDLE hWrite,HANDLE hRead,unsigned __int8 u8Cmd,unsigned __int8 *pu8Data,unsigned __int8 u8Len)
{
    if(NULL==hWrite|| INVALID_HANDLE_VALUE==hWrite || NULL==hRead || INVALID_HANDLE_VALUE==hRead)
    {
        return FALSE;
    }
    DWORD dwBytesWritten=0;
    DWORD dwBytesRead=0;
    unsigned char au8BufOut[65]; // endpoint size + 1
    unsigned char au8BufIn[65]; // endpoint size + 1
    
    au8BufOut[0]=0; // First byte is report ID. Ignored by HID Class firmware as only config'd for one report
    au8BufOut[1]=u8Cmd;    
    WriteFile(hWrite,au8BufOut,65,&dwBytesWritten,0);
    Sleep(10);
    ReadFile(hRead,au8BufIn,65,&dwBytesRead,0);
    memcpy(pu8Data,au8BufIn+3,u8Len);
    return au8BufIn[2];
}

BOOL CDongle::HIDQuery(HANDLE hWrite,HANDLE hRead,unsigned __int8 *pu8Data,unsigned __int8 u8Len)
{
    if(NULL==hWrite|| INVALID_HANDLE_VALUE==hWrite || NULL==hRead || INVALID_HANDLE_VALUE==hRead)
    {
        return FALSE;
    }
    DWORD dwBytesWritten=0;
    DWORD dwBytesRead=0;
    unsigned char au8BufOut[65]; // endpoint size + 1
    unsigned char au8BufIn[65]; // endpoint size + 1

    // Clean out any outstanding reads
    
    au8BufOut[0]=0; // First byte is report ID. Ignored by HID Class firmware as only config'd for one report
    au8BufOut[1]=FCD_HID_CMD_QUERY;
    WriteFile(hWrite,au8BufOut,65,&dwBytesWritten,0);
    Sleep(10);
    ReadFile(hRead,au8BufIn,65,&dwBytesRead,0);
    memcpy(pu8Data,au8BufIn+3,u8Len);
    return au8BufIn[2]==TRUE;
}

BOOL CDongle::Exists()
{
    BOOL retval = FALSE;

    HANDLE hWrite=INVALID_HANDLE_VALUE;
    HANDLE hRead=INVALID_HANDLE_VALUE;
    TCHAR szDevId[200];

    for(int i=0;i<FCD_DEVICE_ID_COUNT;++i)
    {
        if (HIDOpen(m_szVIDPID[i],szDevId,&hWrite,&hRead))
        {
            unsigned char szVersionRead[60];
            if (HIDQuery(hWrite,hRead,szVersionRead,60)==TRUE)
            {
                if (memcmp(szVersionRead,"FCDAPP",6)==0)
                {
                    retval = TRUE;
                }
            }
            HIDClose(hWrite,hRead);
        }        
    }

    return retval;
}

BOOL CDongle::SetFrequency(ULONG frequency)
{
    BOOL retval = FALSE;

    HANDLE hWrite=INVALID_HANDLE_VALUE;
    HANDLE hRead=INVALID_HANDLE_VALUE;
    TCHAR szDevId[200];

    for(int i=0;i<FCD_DEVICE_ID_COUNT;++i)
    {
        if (HIDOpen(m_szVIDPID[i],szDevId,&hWrite,&hRead))
        {
            if(HIDSetFreq(hWrite,hRead,(int)frequency)==TRUE)
            {
                retval = TRUE;
            }
            HIDClose(hWrite,hRead);
        }        
    }

    return retval;
}

ULONG CDongle::GetFrequency()
{
    ULONG retval = 0;

    HANDLE hWrite=INVALID_HANDLE_VALUE;
    HANDLE hRead=INVALID_HANDLE_VALUE;
    TCHAR szDevId[200];

    for(int i=0;i<FCD_DEVICE_ID_COUNT;++i)
    {
        if (HIDOpen(m_szVIDPID[i],szDevId,&hWrite,&hRead))
        {
            retval = HIDGetFreq(hWrite,hRead);
            HIDClose(hWrite,hRead);
        }
    }

    return retval;
}


BOOL CDongle::BiasTEnable(BOOL enable)
{
    BOOL retval = FALSE;

    HANDLE hWrite=INVALID_HANDLE_VALUE;
    HANDLE hRead=INVALID_HANDLE_VALUE;
    TCHAR szDevId[200];

    for(int i=0;i<FCD_DEVICE_ID_COUNT;++i)
    {
        if (HIDOpen(m_szVIDPID[i],szDevId,&hWrite,&hRead))
        {
            retval = HIDSetBiasT(hWrite,hRead, enable?1:0)==1?TRUE:FALSE;
            HIDClose(hWrite,hRead);
        }
    }

    return retval;
}
#else
// Delegate pretty much everything to fcd.c
BOOL CDongle::Exists()
{
	if (fcdGetMode() != FCD_MODE_APP)
		return FALSE;
	return TRUE;
}

BOOL CDongle::SetFrequency(ULONG frequency)
{
	if (Exists())
	{
		return FCD_MODE_APP == fcdAppSetFreq((int)frequency);
	}
	return FALSE;
}

ULONG CDongle::GetFrequency()
{
	ULONG rv = 0;
	if (Exists())
	{
		uint8_t buf[4];
		if (FCD_MODE_APP == fcdAppGetParam(FCD_CMD_APP_GET_FREQ_HZ, buf, 4))
		{
			// convert from little-endian uint32_t in the buffer to ULONG native
			rv = ((ULONG)buf[0]) +
				((ULONG)buf[1] << 8) +
				((ULONG)buf[2] << 16) +
				((ULONG)buf[3] << 24);
		}
	}
	return rv;
}

BOOL CDongle::BiasTEnable(BOOL enable)
{
	if (Exists())
	{
		uint8_t bias = enable ? 1 : 0;
		if (FCD_MODE_APP == fcdAppSetParam(FCD_CMD_APP_SET_BIAS_TEE, &bias, 1))
		{
			return TRUE;
		}
	}
	return FALSE;
}
#endif