/* { dg-message "note: file" "" } */
/* { dg-do compile } */
/* { dg-options "-O -fprofile-use -fopt-info" } */
/* { dg-options "-O -m4 -fprofile-use -fopt-info" { target sh-*-* } } */

void foo (int *p)
{
  if (p)
    *p = 0;
} 
