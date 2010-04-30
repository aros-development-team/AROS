/* Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _FENV_H
#define _FENV_H	1

/* Define bits representing the exception.  We use the bit positions of
   the appropriate bits in the FPSR Accrued Exception Byte.  */
enum
  {
    FE_INEXACT = 1 << 3,
#define FE_INEXACT	FE_INEXACT
    FE_DIVBYZERO = 1 << 4,
#define FE_DIVBYZERO	FE_DIVBYZERO
    FE_UNDERFLOW = 1 << 5,
#define FE_UNDERFLOW	FE_UNDERFLOW
    FE_OVERFLOW = 1 << 6,
#define FE_OVERFLOW	FE_OVERFLOW
    FE_INVALID = 1 << 7
#define FE_INVALID	FE_INVALID
  };

#define FE_ALL_EXCEPT \
	(FE_INEXACT | FE_DIVBYZERO | FE_UNDERFLOW | FE_OVERFLOW | FE_INVALID)

/* The m68k FPU supports all of the four defined rounding modes.  We use
   the bit positions in the FPCR Mode Control Byte as the values for the
   appropriate macros.  */
enum
  {
    FE_TONEAREST = 0,
#define FE_TONEAREST	FE_TONEAREST
    FE_TOWARDZERO = 1 << 4,
#define FE_TOWARDZERO	FE_TOWARDZERO
    FE_DOWNWARD = 2 << 4,
#define FE_DOWNWARD	FE_DOWNWARD
    FE_UPWARD = 3 << 4
#define FE_UPWARD	FE_UPWARD
  };


/* Type representing exception flags.  */
typedef unsigned int fexcept_t;


/* Type representing floating-point environment.  This structure
   corresponds to the layout of the block written by `fmovem'.  */
typedef struct
  {
    unsigned int __control_register;
    unsigned int __status_register;
    unsigned int __instruction_address;
  }
fenv_t;

/* If the default argument is used we use this value.  */
#define FE_DFL_ENV	((__const fenv_t *) -1)

#ifdef __USE_GNU
/* Floating-point environment where none of the exceptions are masked.  */
# define FE_NOMASK_ENV	((__const fenv_t *) -2)
#endif

/* Floating-point exception handling.  */

/* Clear the supported exceptions represented by EXCEPTS.  */
int feclearexcept (int __excepts);

/* Store implementation-defined representation of the exception flags
   indicated by EXCEPTS in the object pointed to by FLAGP.  */
int fegetexceptflag (fexcept_t *__flagp, int __excepts);

/* Raise the supported exceptions represented by EXCEPTS.  */
int feraiseexcept (int __excepts);

/* Set complete status for exceptions indicated by EXCEPTS according to
   the representation in the object pointed to by FLAGP.  */
int fesetexceptflag(const fexcept_t *flagp, int excepts);

/* Determine which of subset of the exceptions specified by EXCEPTS are
   currently set.  */
int fetestexcept (int __excepts);


/* Rounding control.  */

/* Get current rounding direction.  */
int fegetround (void);

/* Establish the rounding direction represented by ROUND.  */
int fesetround (int __rounding_direction);


/* Floating-point environment.  */

/* Store the current floating-point environment in the object pointed
   to by ENVP.  */
int fegetenv (fenv_t *envp);

/* Save the current environment in the object pointed to by ENVP, clear
   exception flags and install a non-stop mode (if available) for all
   exceptions.  */
int feholdexcept (fenv_t *envp);

/* Establish the floating-point environment represented by the object
   pointed to by ENVP.  */
int fesetenv(const fenv_t *envp);

/* Save current exceptions in temporary storage, install environment
   represented by object pointed to by ENVP and raise exceptions
   according to saved exceptions.  */
int feupdateenv(const fenv_t *envp);


#ifdef __USE_GNU

/* Enable individual exceptions.  Will not enable more exceptions than
   EXCEPTS specifies.  Returns the previous enabled exceptions if all
   exceptions are successfully set, otherwise returns -1.  */
int feenableexcept (int __excepts);

/* Disable individual exceptions.  Will not disable more exceptions than
   EXCEPTS specifies.  Returns the previous enabled exceptions if all
   exceptions are successfully disabled, otherwise returns -1.  */
int fedisableexcept (int __excepts);

/* Return enabled exceptions.  */
int fegetexcept (void);
#endif

__END_DECLS

#endif /* fenv.h */

