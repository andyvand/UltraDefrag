#!/usr/local/bin/lua
--[[
  patch-wxwidgets.lua - patches wxWidgets sources for their compilation by WDK 7.
  Copyright (c) 2012 Dmitri Arkhangelski (dmitriar@gmail.com).

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
--]]

-- NOTES for C programmers: 
--   1. the first element of each array has index 1.
--   2. only nil and false values are false, all other including 0 are true

-- USAGE: lua patch-wxwidgets.lua <wxWidgets path>

-- parse parameters
assert(arg[1],"wxWidgets path must be specified!")

-- read the entire /src/common/stopwatch.cpp file
path = arg[1] .. "\\src\\common\\stopwatch.cpp"
f = assert(io.open(path, "r"))
contents = f:read("*all")
f:close()

-- replace 'struct timeb' by 'struct _timeb' definition
new_contents = string.gsub(contents,"struct timeb","struct _timeb")

-- replace 'ftime' by '_ftime' definition
new_contents = string.gsub(contents,"::ftime","::_ftime")

-- save the updated contents
f = assert(io.open(path, "w"))
f:write(new_contents)
f:close()
