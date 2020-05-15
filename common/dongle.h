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

/// @file Dongle.h
//
//////////////////////////////////////////////////////////////////////
#pragma once

#if defined (WIN32) || defined(WIN64)
#include "windows.h"
#endif

// CDongle
class CDongle 
{
public:
    CDongle()
    {
    }
public:

private:
#if defined (WIN32) || defined(WIN64)
    BOOL HIDOpen(PTCHAR pszDevSearchID,PTCHAR pszDevID,PHANDLE phWrite,PHANDLE phRead);
    void HIDClose(HANDLE hWrite,HANDLE hRead);
    unsigned char HIDSetFreq(HANDLE hWrite,HANDLE hRead,int nFreq);
    ULONG HIDGetFreq(HANDLE hWrite,HANDLE hRead);
    unsigned char HIDResetFCD(HANDLE hWrite,HANDLE hRead);
    unsigned char HIDSetBiasT(HANDLE hWrite, HANDLE hRead, unsigned __int8 u8Val);
    BOOL HIDReadCommand(HANDLE hWrite,HANDLE hRead,unsigned __int8 u8Cmd,unsigned __int8 *pu8Data,unsigned __int8 u8Len);
    BOOL HIDQuery(HANDLE hWrite,HANDLE hRead,unsigned __int8 *pu8Data,unsigned __int8 u8Len);
#endif

public:
    BOOL Exists();
    BOOL SetFrequency(ULONG frequency);
    ULONG GetFrequency();
    BOOL BiasTEnable(BOOL enable);
};
