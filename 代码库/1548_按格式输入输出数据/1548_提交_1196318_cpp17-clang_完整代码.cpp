#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

static int hexval(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return 10 + c - 'a';
    if ('A' <= c && c <= 'F') return 10 + c - 'A';
    return -1;
}

// 解析为无符号 128 位十六进制（不接受负号）
static int parse_uint128_hex(const char *s, uint128_t *out, int *hex_digits) {
    const char *p = s;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '+') p++;
    if (*p == '-') return 0; // 不接受负数
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
    uint128_t val = 0;
    int found = 0;
    int digits = 0;
    while (*p) {
        int v = hexval(*p);
        if (v < 0) break;
        val = (val << 4) | (uint128_t)v;
        found = 1;
        digits++;
        p++;
    }
    if (!found) return 0;
    *out = val;
    if (hex_digits) *hex_digits = digits;
    return 1;
}

// 无符号 128 -> 十进制字符串
static void uint128_to_dec(uint128_t uv, char *buf, size_t buflen) {
    if (buflen == 0) return;
    char tmp[80];
    size_t pos = 0;
    if (uv == 0) tmp[pos++] = '0';
    while (uv != 0) {
        int digit = (int)(uv % 10);
        tmp[pos++] = '0' + digit;
        uv /= 10;
    }
    size_t needed = pos + 1;
    if (needed > buflen) { /* 截断，保留末尾 */ pos = buflen - 1; }
    for (size_t i = 0; i < pos; ++i) buf[i] = tmp[pos - 1 - i];
    buf[pos] = '\0';
}

// 无符号 128 -> 十六进制字符串（可带 0x 前缀）
static void uint128_to_hex(uint128_t uv, char *buf, size_t buflen, int with_prefix) {
    if (buflen == 0) return;
    char tmp[80];
    size_t pos = 0;
    if (uv == 0) tmp[pos++] = '0';
    while (uv != 0) {
        int d = (int)(uv & 0xF);
        tmp[pos++] = "0123456789abcdef"[d];
        uv >>= 4;
    }
    size_t prefix = with_prefix ? 2 : 0;
    size_t needed = pos + prefix + 1;
    if (needed > buflen) pos = buflen - prefix - 1;
    char *q = buf;
    if (with_prefix) { *q++ = '0'; *q++ = 'x'; }
    for (size_t i = 0; i < pos; ++i) q[i] = tmp[pos - 1 - i];
    q[pos] = '\0';
}

int main(void) {
    char buf[256];
    int a;
    char bStr[128];
    char s[64];
    double f;
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    if (sscanf(buf, "%d,%127[^,],%63[^,],%lf", &a, bStr, s, &f) < 4) return 0;

    uint128_t b;
    int hex_digits = 0;
    if (!parse_uint128_hex(bStr, &b, &hex_digits)) return 0;

    char b_dec[80];
    uint128_to_dec(b, b_dec, sizeof(b_dec));

    // 打印第一个字段 a（宽度15）
    printf("%15d\n", a);

    // 如果十六进制位数 <= 8，按 32 位带符号输出（0xFFFFFFFF -> -1）
    if (hex_digits > 0 && hex_digits <= 8) {
        int32_t sb = (int32_t)(uint32_t)b;
        printf("%-15d\n", sb);
    } else {
        printf("%-15s\n", b_dec);
    }

    printf("%15s\n%-15.2f\n", s, f);
    return 0;
}