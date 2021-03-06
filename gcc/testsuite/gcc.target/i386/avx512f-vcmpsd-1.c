/* { dg-do compile } */
/* { dg-options "-O2 -mavx512f" } */
/* { dg-final { scan-assembler "vcmpsd\[ \\t\]+\[^\n\]*\[^\}\]%xmm\[0-9\]\[^\n^k\]*%k\[1-7\]\[^\{\]" } } */
/* { dg-final { scan-assembler "vcmpsd\[ \\t\]+\[^\n\]*\[^\}\]%xmm\[0-9\]\[^\n^k\]*%k\[1-7\]\{" } } */
/* { dg-final { scan-assembler "vcmpsd\[ \\t\]+\[^\n\]*\{sae\}\[^\n\]*%xmm\[0-9\]\[^\n^k\]*%k\[1-7\]\[^\{\]" } } */
/* { dg-final { scan-assembler "vcmpsd\[ \\t\]+\[^\n\]*\{sae\}\[^\n\]*%xmm\[0-9\]\[^\n^k\]*%k\[1-7\]\{" } } */

#include <immintrin.h>

volatile __m128d x;
volatile __mmask8 m;

void extern
avx512f_test (void)
{
  m = _mm_cmp_sd_mask (x, x, _CMP_FALSE_OQ);
  m = _mm_mask_cmp_sd_mask (m, x, x, _CMP_FALSE_OQ);
  m = _mm_cmp_round_sd_mask (x, x, _CMP_FALSE_OQ, _MM_FROUND_NO_EXC);
  m = _mm_mask_cmp_round_sd_mask (m, x, x, _CMP_FALSE_OQ, _MM_FROUND_NO_EXC);
}
