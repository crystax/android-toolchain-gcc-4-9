/* { dg-do compile { target { ! ia32 } } } */
/* { dg-options "-O2 -mbmi -fno-inline -dp  --param max-default-completely-peeled-insns=0" } */

#include "bmi-blsr-1.c"

/* { dg-final { scan-assembler-times "bmi_blsr_di" 1 } } */
