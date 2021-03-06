/* { dg-do compile } */
/* { dg-options "-O2" } */
/* { dg-final { scan-assembler "and" { target powerpc*-*-* spu-*-* } } } */
/* There should be no nand for this testcase for SPU. */
/* { dg-final { scan-assembler-not "nand" { target spu-*-* } } } */

int f(int y)
{
  return y & ~(y & -y);
}
