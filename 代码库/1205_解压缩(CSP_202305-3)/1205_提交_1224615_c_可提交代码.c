#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int main(void) {
    size_t s;
    if (scanf("%zu", &s) != 1) return 0;

    // 读取十六进制串为压缩字节流
    unsigned char *in = (unsigned char *)malloc(s);
    if (!in) return 0;
    size_t cnt = 0;
    int hi = -1, ch;
    while (cnt < s && (ch = getchar()) != EOF) {
        int v = hexval(ch);
        if (v < 0) continue;
        if (hi < 0) hi = v;
        else {
            in[cnt++] = (unsigned char)((hi << 4) | v);
            hi = -1;
        }
    }
    if (cnt != s) { free(in); return 0; } // 输入保证合法

    size_t p = 0;

    // 读取 Snappy varint：原始数据长度
    uint64_t ulen = 0; int shift = 0;
    while (1) {
        unsigned char b = in[p++];
        ulen |= (uint64_t)(b & 0x7F) << shift;
        if ((b & 0x80) == 0) break;
        shift += 7;
    }
    size_t out_cap = (size_t)ulen;
    unsigned char *out = (unsigned char *)malloc(out_cap);
    if (!out) { free(in); return 0; }
    size_t out_len = 0;

    while (out_len < out_cap) {
        unsigned char tag = in[p++];
        int type = tag & 0x3;

        if (type == 0) {
            // LITERAL
            size_t len;
            int x = tag >> 2;
            if (x <= 59) {
                len = (size_t)x + 1;
            } else {
                int nbytes = x - 59; // 1..4
                uint64_t v = 0;
                for (int i = 0; i < nbytes; ++i) v |= (uint64_t)in[p++] << (8 * i);
                len = (size_t)(v + 1);
            }
            memcpy(out + out_len, in + p, len);
            p += len;
            out_len += len;
        } else if (type == 1) {
            // COPY_1：长度3位+4；偏移 = 高3位<<8 | 下一字节
            size_t len = ((tag >> 2) & 0x7) + 4;
            size_t off = ((size_t)(tag & 0xE0) << 3) | in[p++];
            for (size_t i = 0; i < len; ++i) {
                out[out_len] = out[out_len - off];
                ++out_len;
            }
        } else if (type == 2) {
            // COPY_2：长度=高6位+1；偏移=LE16
            size_t len = (tag >> 2) + 1;
            size_t off = (size_t)in[p] | ((size_t)in[p + 1] << 8);
            p += 2;
            for (size_t i = 0; i < len; ++i) {
                out[out_len] = out[out_len - off];
                ++out_len;
            }
        } else {
            // COPY_4：长度=高6位+1；偏移=LE32
            size_t len = (tag >> 2) + 1;
            size_t off = (size_t)in[p] |
                         ((size_t)in[p + 1] << 8) |
                         ((size_t)in[p + 2] << 16) |
                         ((size_t)in[p + 3] << 24);
            p += 4;
            for (size_t i = 0; i < len; ++i) {
                out[out_len] = out[out_len - off];
                ++out_len;
            }
        }
    }

    // 按每行 8 字节输出（小写十六进制）
    for (size_t i = 0; i < out_cap; ++i) {
        printf("%02x", out[i]);
        if ((i & 7) == 7) putchar('\n');
    }
    if ((out_cap & 7) != 0) putchar('\n');

    free(in);
    free(out);
    return 0;
}