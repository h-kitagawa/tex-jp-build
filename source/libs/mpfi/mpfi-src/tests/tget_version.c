/* tget_version.c -- Test mpfi_get_version.

Copyright 2009, 2010,
                     Spaces project, Inria Lorraine
                     and Salsa project, INRIA Rocquencourt,
                     and Arenaire project, Inria Rhone-Alpes, France
                     and Lab. ANO, USTL (Univ. of Lille),  France


This file is part of the MPFI Library.

The MPFI Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The MPFI Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the MPFI Library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
MA 02110-1301, USA. */

#include "mpfi-tests.h"

int
main (int argc, char **argv)
{
  if (strcmp (mpfi_get_version (), PACKAGE_VERSION) != 0) {
    printf ("Error: The value return by mpfi_get_version (\"%s\") and the "
            "one of the VERSION symbol (" QUOTE(PACKAGE_VERSION) ") differ.\n",
            mpfi_get_version ());
    exit (1);
  }

  printf ("MPFI version: %s\n", mpfi_get_version ());
  printf ("MPFR version: %s\n", mpfr_get_version ());
  printf ("GMP  version: %s\n", gmp_version);

  return 0;
}
