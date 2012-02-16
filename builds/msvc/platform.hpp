/*
    Copyright (c) 2012 250bpm s.r.o.
    Copyright (c) 2007-2011 iMatix Corporation
    Copyright (c) 2007-2011 Other contributors as noted in the AUTHORS file

    This file is part of Crossroads I/O project.

    Crossroads I/O is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Crossroads is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __XS_PLATFORM_HPP_INCLUDED__
#define __XS_PLATFORM_HPP_INCLUDED__

//  This is the platform definition for the MSVC platform.
//  As a first step of the build process it is copied to
//  src directory to take place of platform.hpp generated from
//  platform.hpp.in on platforms supported by GNU autotools.

#define XS_HAVE_WINDOWS

#define XS_HAVE_SELECT

#endif
