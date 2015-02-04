# This is Picprog, Microchip PIC programmer software for the serial
# port device.
# Copyright © 1997,2002,2003,2004 Jaakko Hyvätti
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
# 
# The author may be contacted at:
# 
# Email: Jaakko.Hyvatti@iki.fi
# URL:   http://www.iki.fi/hyvatti/
# Phone: +358 40 5011222
# 
# Please send any suggestions, bug reports, success stories etc. to the
# Email address above.

# Please use a reasonably recent GNU make.

CXX=g++
CXXFLAGS=-O2 -Wall -W -Wwrite-strings
LDFLAGS=-s -lusb

OBJS=main.o picport.o hexfile.o program.o ser_avrdoper.o
PROG=picprog

all: $(PROG)

$(PROG): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

dep:
	$(CXX) -M $(CXXFLAGS) *.cc > .depend

clean:
	rm -f core $(OBJS) $(PROG)

tar:
	chmod -R a+rX,go-w .
	VERSION=`expr $$PWD : '.*-\([1-9]\.[1-9]\?[0-9]\(\.[0-9]\)\?\)$$'`; \
	cd ..; \
	tar -czvf $(PROG)-$$VERSION.tar.gz \
		$(PROG)-$$VERSION/{Makefile,COPYING,README} \
		$(PROG)-$$VERSION/[a-zA-Z]*.{html,png,1,h,cc}

install: all
	install -c -o 0 -g 0 -m 755 $(PROG) /usr/local/bin/
	install -c -o 0 -g 0 -m 644 *.1 /usr/local/man/man1/

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
