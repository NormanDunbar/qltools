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


ifeq ($(OSTYPE),cygwin32)
	DIR=NT
else	
	DIR=Unix	
endif

all: domake

domake:
	cd $(DIR) ; $(MAKE) 


unix:	
	cd Unix ; $(MAKE)

nt:
	cd NT ;  $(MAKE)

dos:
	cd msdos ; $(MAKE)

clean:
	-rm -f *~
	-rm -f '#*#'
	cd $(DIR) ; $(MAKE) clean 

realclean:
	-rm -f *~
	-rm -f '#*#'
	cd $(DIR) ; $(MAKE) realclean 

