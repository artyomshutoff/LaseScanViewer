#pragma once
#include <cstddef>
// SSE2 is part of the x64 baseline. Four independent sums accelerate the
// descriptor norm; only final floating-point rounding can differ. No AVX/FMA.
inline double descriptorDistance190(const double* a,const double* b){
#if defined(__x86_64__) && defined(__clang__) && !defined(DISABLE_ASM_DISTANCE)
    double sum;size_t offset;
    __asm__(
        "pxor %[sum], %[sum]\n\t"
        "pxor %%xmm3, %%xmm3\n\t"
        "xor %[offset], %[offset]\n\t"
        "1:\n\t"
        "movupd (%[a],%[offset],8), %%xmm1\n\t"
        "movupd (%[b],%[offset],8), %%xmm2\n\t"
        "subpd %%xmm2, %%xmm1\n\t"
        "mulpd %%xmm1, %%xmm1\n\t"
        "addpd %%xmm1, %[sum]\n\t"
        "movupd 16(%[a],%[offset],8), %%xmm1\n\t"
        "movupd 16(%[b],%[offset],8), %%xmm2\n\t"
        "subpd %%xmm2, %%xmm1\n\t"
        "mulpd %%xmm1, %%xmm1\n\t"
        "addpd %%xmm1, %%xmm3\n\t"
        "add $4, %[offset]\n\t"
        "cmp $188, %[offset]\n\t"
        "jne 1b\n\t"
        "movupd (%[a],%[offset],8), %%xmm1\n\t"
        "movupd (%[b],%[offset],8), %%xmm2\n\t"
        "subpd %%xmm2, %%xmm1\n\t"
        "mulpd %%xmm1, %%xmm1\n\t"
        "addpd %%xmm1, %[sum]\n\t"
        "addpd %%xmm3, %[sum]\n\t"
        "movapd %[sum], %%xmm1\n\t"
        "unpckhpd %%xmm1, %%xmm1\n\t"
        "addsd %%xmm1, %[sum]\n\t"
        : [sum] "=&x" (sum),[offset] "=&r" (offset)
        : [a] "r" (a),[b] "r" (b)
        : "xmm1","xmm2","xmm3","cc","memory");
    return sum;
#else
    double sum=0;for(int i=0;i<190;i++){double d=a[i]-b[i];sum+=d*d;}return sum;
#endif
}
