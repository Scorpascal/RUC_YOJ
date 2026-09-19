#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define BASE 1000000000U  // 1e9

typedef struct {
    uint32_t *d;
    int len;
    int cap;
} Big;

// 确保容量
void ensure(Big *b, int need) {
    if (need <= b->cap) return;
    int ncap = b->cap ? b->cap : 4;
    while (ncap < need) ncap <<= 1;
    b->d = (uint32_t*)realloc(b->d, sizeof(uint32_t)*ncap);
    b->cap = ncap;
}

// 初始化为一个小整数
void init_big(Big *b, uint64_t v) {
    b->d = NULL;
    b->len = 0;
    b->cap = 0;
    if (v == 0) {
        ensure(b,1);
        b->d[0]=0;
        b->len=1;
        return;
    }
    while (v) {
        ensure(b, b->len+1);
        b->d[b->len++] = (uint32_t)(v % BASE);
        v /= BASE;
    }
}

// 复制
void copy_big(Big *dst, const Big *src) {
    ensure(dst, src->len);
    for (int i=0;i<src->len;i++) dst->d[i]=src->d[i];
    dst->len = src->len;
}

// 去掉前导 0
void norm(Big *b) {
    while (b->len>1 && b->d[b->len-1]==0) b->len--;
}

// 乘 2
void mul2(Big *b) {
    uint64_t carry = 0;
    for (int i=0;i<b->len;i++) {
        uint64_t cur = (uint64_t)b->d[i]*2 + carry;
        b->d[i] = (uint32_t)(cur % BASE);
        carry = cur / BASE;
    }
    if (carry) {
        ensure(b,b->len+1);
        b->d[b->len++] = (uint32_t)carry;
    }
}

// 加一个 64 位小整数
void add_small(Big *b, uint64_t v) {
    int i = 0;
    uint64_t carry = v;
    while (carry) {
        if (i >= b->len) {
            ensure(b, b->len+1);
            b->d[b->len++] = 0;
        }
        uint64_t cur = (uint64_t)b->d[i] + (carry % BASE);
        carry /= BASE;
        if (cur >= BASE) {
            cur -= BASE;
            carry += 1;
        }
        b->d[i] = (uint32_t)cur;
        i++;
    }
}

// 输出
void print_big(const Big *b) {
    printf("%u", b->d[b->len-1]);
    for (int i=b->len-2;i>=0;i--) {
        printf("%09u", b->d[i]);
    }
    printf("\n");
}

int main() {
    int n;
    if (scanf("%d",&n)!=1) return 0;
    if (n==1) {
        printf("1\n");
        return 0;
    }

    Big S;          // S(i-1)
    init_big(&S, 1); // S(1)=1

    for (int i=2;i<=n;i++) {
        uint64_t sq = (uint64_t)i * (uint64_t)i;
        if (i == n) {
            Big Fn;
            init_big(&Fn,0);
            copy_big(&Fn, &S);    // Fn = S(n-1)
            add_small(&Fn, sq);   // Fn += n^2
            print_big(&Fn);
            free(Fn.d);
            break;
        }
        // 更新 S -> S(i) = 2*S(i-1) + i^2
        mul2(&S);
        add_small(&S, sq);
    }

    free(S.d);
    return 0;
}