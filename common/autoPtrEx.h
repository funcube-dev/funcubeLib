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

/// @file AutoPtrEx.h
//
//////////////////////////////////////////////////////////////////////

#ifndef __AUTOPTREX_H__
#define __AUTOPTREX_H__

#if defined(_WIN32) || defined(_WIN64)
#include <excpt.h>
#include <xstddef>
#endif

namespace fc
{

/** @class auto_ptr_ex
    Extends the normal behaviour of an auto_ptr, to set the contained
    pointer to NULL when not owned
*/
template<class _Ty>
class auto_ptr_ex
{
public:
    typedef _Ty element_type;
    explicit auto_ptr_ex(_Ty *_P = 0) noexcept : _Owns(_P != 0), _Ptr(_P) {}
    auto_ptr_ex(const auto_ptr_ex<_Ty>& _Y) noexcept    : _Owns(_Y._Owns), _Ptr(_Y.release()) {}

    auto_ptr_ex<_Ty>& operator=(const auto_ptr_ex<_Ty>& _Y) noexcept
    {
        if (this != &_Y)
        {
            if (_Ptr != _Y.get())
            {
                if (_Owns)
                    delete _Ptr;
                _Owns = _Y._Owns; 
            }
            else if (_Y._Owns)
                _Owns = true;
            _Ptr = _Y.release();
        }
        return (*this); 
    }
    ~auto_ptr_ex()
    {
        if (_Owns)
            delete _Ptr; 
    }
    _Ty& operator*() const noexcept { return (*get()); }
    _Ty *operator->() const noexcept { return (get()); }
    _Ty *get() const noexcept { return (_Ptr); }
    _Ty *release() const noexcept
    { 
        _Ty *_Tmp=_Ptr;
        ((auto_ptr_ex<_Ty> *)this)->_Owns = false;
        ((auto_ptr_ex<_Ty> *)this)->_Ptr = 0;
        return (_Tmp); 
    }
private:
    bool _Owns;
    _Ty *_Ptr;
};

}

#endif // __AUTOPTREX_H__

