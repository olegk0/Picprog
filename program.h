/* -*- c++ -*-

This is a generic program misc functions class v1.0.
Copyright © 1997,2002,2003,2004,2010 Jaakko Hyvätti

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see http://www.gnu.org/licenses/ .

The author may be contacted at:

Email: Jaakko.Hyvatti@iki.fi
URL:   http://www.iki.fi/hyvatti/
Phone: +358 40 5011222

Please send any suggestions, bug reports, success stories etc. to the
Email address above.  Include word 'picprog' in the subject line to
make sure your email passes my spam filtering.

*/

#ifndef H_PROGRAM
#define H_PROGRAM

#include <getopt.h>

class program {
  static const char warranty_t [];
  static const char copying_t [];
public:
  char *name;
  program ();
  ~program ();
  void init (char *argv []);
  void warranty ();
  void copying ();
  void usage (struct option *long_opts, char *short_opts);
};

#endif // H_PROGRAM
