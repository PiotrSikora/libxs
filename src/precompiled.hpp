/*
    Copyright (c) 2011-2012 250bpm s.r.o.
    Copyright (c) 2011 Other contributors as noted in the AUTHORS file

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

#ifndef __XS_PRECOMPILED_HPP_INCLUDED__
#define __XS_PRECOMPILED_HPP_INCLUDED__

#ifdef _MSC_VER

// Windows headers
#include "platform.hpp"
#include "windows.hpp"
#include <fcntl.h>
#include <intrin.h>
#include <io.h>
#include <rpc.h>
#include <sys/stat.h>

// standard C++ headers
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

// Crossroads definitions and exported functions
#include "../include/xs.h"

#endif // _MSC_VER

#endif
