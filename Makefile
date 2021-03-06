################################################################
#  Makefile
# 
# 
#  Copyright (C) 1999 Jonathan R. Hudson
#  Developed by Jonathan R. Hudson <jrhudson@bigfoot.com>
# 
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
# 
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# 
################################################################
EXE_EXT = 
CC = gcc

SYS := $(shell $(CC) -dumpmachine)
ifneq (, $(findstring linux, $(SYS)))
	EXE_EXT =
else ifneq (, $(findstring mingw, $(SYS)))
	EXE_EXT = .exe
endif

unix:	
	$(MAKE) EXE_EXT=$(EXE_EXT) -C Unix

hxcfe:
	$(MAKE) EXE_EXT=$(EXE_EXT) -C Hxcfe
    
win7:    
	$(MAKE) EXE_EXT=$(EXE_EXT) -C Win7

clean:
	-rm Unix/*.o
	-rm Hxcfe/*.o
	-rm Unix/qltools
	-rm Unix/qltools.exe
	-rm Hxcfe/qltools-hxcfe
	-rm Hxcfe/qltools-hxcfe.exe

