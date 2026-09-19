#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* xstrdup(const char* s) {
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (!p) exit(1);
    memcpy(p, s, n);
    return p;
}

static char* trim_leading_zeros(const char* s) {
    size_t n = strlen(s);
    size_t i = 0;
    while (i + 1 < n && s[i] == '0') i++;
    char* r = (char*)malloc(n - i + 1);
    if (!r) exit(1);
    memcpy(r, s + i, n - i + 1);
    return r;
}

static int big_cmp(const char* a, const char* b) {
    size_t la = strlen(a), lb = strlen(b);
    if (la != lb) return (la < lb) ? -1 : 1;
    int c = strcmp(a, b);
    return (c < 0) ? -1 : (c > 0 ? 1 : 0);
}

static char* big_sub(const char* a, const char* b) {
    // assume a >= b
    size_t la = strlen(a), lb = strlen(b);
    char* res = (char*)malloc(la + 1);
    if (!res) exit(1);
    res[la] = '\0';
    int i = (int)la - 1, j = (int)lb - 1, k = (int)la - 1;
    int borrow = 0;
    while (i >= 0) {
        int ai = a[i] - '0';
        int bi = (j >= 0) ? (b[j] - '0') : 0;
        int d = ai - borrow - bi;
        if (d < 0) { d += 10; borrow = 1; } else { borrow = 0; }
        res[k] = (char)('0' + d);
        i--; j--; k--;
    }
    // trim leading zeros
    size_t pos = 0;
    while (pos + 1 < la && res[pos] == '0') pos++;
    char* out = xstrdup(res + pos);
    free(res);
    return out;
}

static char* big_mul_small(const char* a, int m) {
    if (m == 0) return xstrdup("0");
    if (m == 1) return xstrdup(a);
    size_t la = strlen(a);
    // maximum la+1 digits
    char* buf = (char*)malloc(la + 2);
    if (!buf) exit(1);
    size_t rpos = la + 1;
    buf[rpos] = '\0'; // put terminator beyond the last possible digit
    int carry = 0;
    for (int i = (int)la - 1; i >= 0; --i) {
        int t = (a[i] - '0') * m + carry;
        buf[--rpos] = (char)('0' + (t % 10));
        carry = t / 10;
    }
    while (carry > 0) {
        buf[--rpos] = (char)('0' + (carry % 10));
        carry /= 10;
    }
    char* out = xstrdup(buf + rpos);
    free(buf);
    return out;
}

static char* mul10_add_digit(const char* r, int d) {
    // compute r*10 + d
    if (r[0] == '0' && r[1] == '\0') {
        char tmp[2] = { (char)('0' + d), '\0' };
        if (d == 0) return xstrdup("0");
        return xstrdup(tmp);
    }
    size_t lr = strlen(r);
    char* out = (char*)malloc(lr + 2);
    if (!out) exit(1);
    memcpy(out, r, lr);
    out[lr] = (char)('0' + d);
    out[lr + 1] = '\0';
    return out;
}

int main(void) {
    char a_in[2048], b_in[2048];
    if (scanf("%2047s%2047s", a_in, b_in) != 2) return 0;

    char* A = trim_leading_zeros(a_in);
    char* B = trim_leading_zeros(b_in);

    // A, B > 0 by problem statement, but we still guard B == "0"
    if (B[0] == '0' && B[1] == '\0') {
        // undefined; but avoid division by zero
        printf("0\n");
        free(A); free(B);
        return 0;
    }

    if (big_cmp(A, B) < 0) {
        printf("%s\n", A);
        free(A); free(B);
        return 0;
    }

    char* R = xstrdup("0");
    size_t lenA = strlen(A);
    for (size_t i = 0; i < lenA; ++i) {
        int d = A[i] - '0';
        char* newR = mul10_add_digit(R, d);
        free(R);
        R = newR;

        int lo = 0, hi = 9, best = 0;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            char* t = big_mul_small(B, mid);
            int cmp = big_cmp(t, R);
            if (cmp <= 0) {
                best = mid;
                lo = mid + 1;
            } else {
                hi = mid - 1;
            }
            free(t);
        }
        if (best > 0) {
            char* tb = big_mul_small(B, best);
            char* nr = big_sub(R, tb);
            free(R);
            free(tb);
            R = nr;
        }
    }

    // R already has no前导0
    printf("%s\n", R);

    free(A); free(B); free(R);
    return 0;
}