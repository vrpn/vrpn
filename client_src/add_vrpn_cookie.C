#include <stdio.h>                      // for fprintf, fclose, fopen, etc
#include <stdlib.h>                     // for exit
#include <vrpn_Connection.h>            // for vrpn_cookie_size, etc

static const char DESC[] = "Reads a vrpn log;  writes out the same log with a (current-version) "
	"magic cookie in front.";

// TODO:
//   Smart checking of command-line arguments
//   Check to see if there's already a magic cookie there.

//   ?  Allow the user to specify cookie version inserted (non-current)

void Usage (char * name) {
  fprintf(stderr, "%s\n", DESC);
  fprintf(stderr, "Usage:  %s <input filename> <output filename>\n",
          name);
}

int main (int argc, char ** argv) {

  FILE * f_in;
  FILE * f_out;
  char magicbuf [733];  // HACK
  char a;
  char * in_name;
  char * out_name;
  int final_retval = 0;
  int retval;

  // parse command line arguments

  if (argc != 3) {
    Usage(argv[0]);
    exit(0);
  }

  in_name = argv[1];
  out_name = argv[2];

  f_in = fopen(in_name, "rb");
  if (!f_in) {
    fprintf(stderr, "Couldn't open input file %s.\n", in_name);
    exit(0);
  }

  // check to see if f_out exists
  f_out = fopen(out_name, "rb");
  if (f_out) {
    fprintf(stderr, "Output file \"%s\" already exists.\n", out_name);
    fclose(f_in);
    exit(0);
  }

  f_out = fopen(out_name, "wb");
  if (!f_out) {
    fprintf(stderr, "Couldn't open output file (%s).\n", out_name);
    fclose(f_in);
    exit(0);
  }

  retval = write_vrpn_cookie(magicbuf, vrpn_cookie_size() + 1,
                             vrpn_LOG_NONE);
  if (retval < 0) {
    fprintf(stderr, "vrpn_Connection::close_log:  "
                    "Couldn't create magic cookie.\n");
    goto CLEANUP;
  }

  retval = static_cast<int>(fwrite(magicbuf, 1, vrpn_cookie_size(), f_out));
  if (retval != static_cast<int>(vrpn_cookie_size())) {
    fprintf(stderr, "vrpn_Connection::close_log:  "
                    "Couldn't write magic cookie to log file "
                    "(got %d, expected %d).\n",
            retval, static_cast<int>(vrpn_cookie_size()));
    goto CLEANUP;
  }

  while (!final_retval && !feof(f_in)) {
    a = fgetc(f_in);
    fputc(a, f_out);
  }

CLEANUP:

  fclose(f_out);
  fclose(f_in);

}

