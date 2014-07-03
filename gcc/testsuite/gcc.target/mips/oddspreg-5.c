/* Check that we disable odd-numbered single precision registers and can
   still generate code.  */
/* { dg-options "-mabi=64 -mno-odd-spreg -mhard-float" } */

#if _MIPS_SPFPSET != 32
#error "Incorrect number of single-precision registers reported"
#endif

float a;
float
foo ()
{
  float b = a + 1.0f;
  return b;
}
