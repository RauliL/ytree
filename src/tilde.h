/* tilde.h: Externally available tilde expansion helpers. */

/* Copyright (C) 1992 Free Software Foundation, Inc.

   This file contains the Readline Library (the Library), a set of
   routines for providing Emacs style line input to programs that ask
   for it.

   The Library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   The Library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   The GNU General Public License is often shipped with GNU software, and
   is generally kept in a file called COPYING or LICENSE.  If you do not
   have a copy of the license, write to the Free Software Foundation,
   59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#pragma once

#include <string>

/* Return a new string which is the result of tilde expanding STRING. */
std::string tilde_expand(const std::string& string);

/* Do the work of tilde expansion on FILENAME.  FILENAME starts with a
   tilde. */
std::string tilde_expand_word(const std::string& filename);
