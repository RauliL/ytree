/* xmalloc.c -- safe versions of malloc and realloc */

/* Copyright (C) 1991 Free Software Foundation, Inc.

   This file is part of GNU Readline, a library for reading lines
   of text with interactive input and history editing.

   Readline is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   Readline is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Readline; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#include "ytree.h"

#define READLINE_LIBRARY

#include "xmalloc.h"

/* **************************************************************** */
/*								    */
/*		   Memory Allocation and Deallocation.		    */
/*								    */
/* **************************************************************** */

static void
memory_error_and_abort (const char *fname)
{
  Error("malloc() failed*Abort");
  /* fprintf (stderr, "%s: out of virtual memory\n", fname); */
  std::exit(2);
}

/* Return a pointer to free()able block of memory large enough
   to hold BYTES number of bytes.  If the memory cannot be allocated,
   print an error message and abort. */
PTR_T
xmalloc (size_t bytes)
{
  PTR_T temp;

  temp = malloc (bytes);
  if (temp == 0)
    memory_error_and_abort ("xmalloc");
  return (temp);
}

PTR_T
xrealloc (PTR_T pointer, size_t bytes)
{
  PTR_T temp;

  temp = pointer ? realloc (pointer, bytes) : malloc (bytes);

  if (temp == 0)
    memory_error_and_abort ("xrealloc");
  return (temp);
}

/* Use this as the function to call when adding unwind protects so we
   don't need to know what free() returns. */
void
xfree (PTR_T string)
{
  if (string)
    free (string);
}
