/* from
    https://github.com/kshitijl/avx2-examples/blob/master/examples/04-dot-product.c
    */

/* Compute the dot product of two (properly aligned) vectors. */
#include <stdio.h>
#include <immintrin.h>
#include <math.h>

const int N = 83;

double slow_dot_product(const double *a, const double *b) {
  double answer = 0.0;
  for(int ii = 0; ii < N; ++ii)
    answer += a[ii]*b[ii];
  return answer;
}

int main() {
  __attribute__ ((aligned (32))) double a[N], b[N];

  for(int ii = 0; ii < N; ++ii)
    a[ii] = b[ii] = ii/sqrt(N);

  printf("calculating dot product without avx..\n");
  printf("result = %f\n", slow_dot_product(a,b));
}
