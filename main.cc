/* -*- c++ -*-

This is Picprog, Microchip PIC programmer software for the serial port device.
Copyright © 1997,2002,2003,2004,2006,2008,2010 Jaakko Hyvätti

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

#include <iostream>
#include <cstring>
#include <cstdlib>

#include <sysexits.h>
#include <unistd.h>
#include <getopt.h>

#include "hexfile.h"
#include "program.h"

using namespace std;

program prog;

char short_opts [] = "d:p:i:o:c:qh?";

static const char *
getenv_default (const char *var, const char *def)
{
  const char *val = getenv (var);
  if (val && *val)
    return val;
  return def;
}

int
main (int argc, char **argv)
{
  int opt_warranty = 0;
  int opt_copying = 0;
  int opt_quiet = 0;
  int opt_usage = 0;

  int opt_format = hexfile::unknown;
//  const char *opt_port = getenv_default ("PIC_PORT", "/dev/ttyS0");
  const char *opt_input = NULL;
  const char *opt_output = NULL;
  const char *opt_cc = NULL;
  int opt_skip = 0;
  int opt_erase = 0;
  int opt_burn = 0;
  int opt_calibration = 0;
  int opt_slow = 0;

//  int opt_hardware = (int)(picport::jdm);

  // Auto ident (-1) is the default, or if that fails, first device (0)
  int opt_device = hexfile::find_device (getenv ("PIC_DEVICE"));

  struct option long_opts[] = {
    {"quiet", no_argument, NULL, 'q'},
    {"warranty", no_argument, &opt_warranty, 1},
    {"copying", no_argument, &opt_copying, 1},
    {"help", no_argument, NULL, 'h'},
    {"device", required_argument, NULL, 'd'},
//    {"pic-serial-port", required_argument, NULL, 'p'},
    {"input-hexfile", required_argument, NULL, 'i'},
    {"output-hexfile", required_argument, NULL, 'o'},
    {"ihx32", no_argument, &opt_format, hexfile::ihx32},
    {"ihx16", no_argument, &opt_format, hexfile::ihx16},
    {"ihx8m", no_argument, &opt_format, hexfile::ihx8m},
    {"cc-hexfile", required_argument, NULL, 'c'},
    {"skip-ones", no_argument, &opt_skip, 1},
    {"erase", no_argument, &opt_erase, 1},
    {"burn", no_argument, &opt_burn, 1},
    {"force-calibration", no_argument, &opt_calibration, 1},
    {"slow", no_argument, &opt_slow, 1},
//    {"jdm", no_argument, &opt_hardware, (int)(picport::jdm)},
//    {"k8048", no_argument, &opt_hardware, (int)(picport::k8048)},
    {0, 0, 0, 0}
  };

  int optc;

  prog.init (argv);

  while (0 <= (optc = getopt_long (argc, argv, short_opts, long_opts, NULL)))
    switch (optc) {
    case 0:
      break;
    case 'd':
      if (!strcmp (optarg, "auto"))
	opt_device = -1;
      else if (-1 == (opt_device = hexfile::find_device (optarg)))
	opt_usage = 1;
      break;
    case 'i':
      opt_input = optarg;
      break;
    case 'o':
      opt_output = optarg;
      break;
    case 'c':
      opt_cc = optarg;
      break;
    case 'q':
      opt_quiet = 1;
      break;
    default: // -? -h --help unknown flag
      opt_usage = 1;
    }

  if (!opt_quiet || opt_warranty || opt_copying || opt_usage)
    // Locale charset should be respected here.
    cerr << "Picprog version 1.9.1, Copyright © 2010 Jaakko Hyvätti <Jaakko.Hyvatti@iki.fi>\n"
      "Picprog comes with ABSOLUTELY NO WARRANTY; for details\n"
      "type `" << prog.name << " --warranty'.  This is free software,\n"
      "and you are welcome to redistribute it under certain conditions;\n"
      "type `" << prog.name << " --copying' for details.\n\n";

  if (opt_copying)
    prog.copying ();

  if (opt_warranty)
    prog.warranty ();

  if (opt_usage) {
    cerr << "Full documentation is at "
      "<URL:http://www.iki.fi/hyvatti/pic/picprog.html>" << endl
	 << "The author may be contacted at:" << endl << endl

	 << "Email: Jaakko.Hyvatti@iki.fi" << endl
	 << "URL:   http://www.iki.fi/hyvatti/" << endl << endl

	 << "Supported devices:" << endl;
    hexfile::print_devices ();
    cerr << endl;

    prog.usage (long_opts, short_opts);
  }

  if (opt_warranty || opt_copying || opt_usage)
    return EX_OK;

  if (!opt_input && !opt_output && !opt_erase) {
    cerr << "Please specify either input or output hexfile or --erase option."
	 << endl;
    prog.usage (long_opts, short_opts);
  }

  if (opt_cc && !opt_input) {
    cerr << "Carbon copy does not make sense without input file." << endl;
    prog.usage (long_opts, short_opts);
  }

  picport pic (opt_slow);
//	       picport::hardware_types(opt_hardware));

  // if both input and output files are specified, first program the device
  // and then read it.

  if (opt_input || opt_erase) {

    hexfile mem;
    int retval;

    if (EX_OK != (retval = mem.setdevice (pic, opt_device)))
      return retval;

    if (opt_input && EX_OK != (retval = mem.load (opt_input)))
      return retval;

    if (opt_burn) {
      if (EX_OK != (retval = mem.program (pic, opt_erase, opt_calibration)))
    	  return retval;
    } else
      cout << "No --burn option specified, device not programmed.\n";



    if (opt_input && opt_cc)
      if (EX_OK != (retval = mem.save (opt_cc,
				       hexfile::formats (opt_format),
				       opt_skip)))
	return retval;
  }

  if (opt_output) {
    hexfile mem;
    int retval;

    if (EX_OK != (retval = mem.setdevice (pic, opt_device)))
      return retval;

    if (EX_OK != (retval = mem.read (pic)))
      return retval;

    if (EX_OK != (retval = mem.save (opt_output,
				     hexfile::formats (opt_format),
				     opt_skip)))
      return retval;
  }

  return EX_OK;
}
