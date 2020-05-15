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

/// @file IAudioSource.h
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "IAudioSink.h"

interface IAudioSource
{
public:    
    virtual BOOL Start(IAudioSink* destination) = 0;
    virtual BOOL IsStarted() = 0;
    virtual BOOL Stop() = 0;
    virtual int Channels() = 0;
    virtual BOOL IsComplexSignal() = 0;
    virtual BOOL IsRealtime() = 0;
};