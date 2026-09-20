#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline char *read_line() {
    size_t cap = 1<<20; // up to 1e6
    char *buf = (char*)malloc(cap+5);
    if (!buf) exit(1);
    if (!fgets(buf, (int)cap, stdin)) { free(buf); return NULL; }
    size_t len = strlen(buf);
    while (len && (buf[len-1]=='\n' || buf[len-1]=='\r')) buf[--len]=0;
    return buf;
}

static void build_lps(const char *p, int m, int *lps) {
    int len = 0; lps[0] = 0;
    for (int i=1;i<m;i++){
        while (len>0 && p[i]!=p[len]) len = lps[len-1];
        if (p[i]==p[len]) len++;
        lps[i]=len;
    }
}

int main() {
    char *T = read_line();
    char *P = read_line();
    if (!T || !P) return 0;
    int n = (int)strlen(T);
    int m = (int)strlen(P);

    // KMP to collect all match starts
    int *lps = (int*)malloc(sizeof(int)*m);
    build_lps(P, m, lps);

    int *starts = (int*)malloc(sizeof(int)*(n+1));
    int sc = 0;
    int j = 0;
    for (int i=0;i<n;i++){
        while (j>0 && T[i]!=P[j]) j = lps[j-1];
        if (T[i]==P[j]) j++;
        if (j==m){
            starts[sc++] = i - m + 1;
            j = lps[j-1];
        }
    }

    // Global greedy selection for full T
    int *chosenStart = (int*)malloc(sizeof(int)*sc);
    int *chosenEnd   = (int*)malloc(sizeof(int)*sc);
    int cc = 0;
    int nextAllowed = 0;
    for (int k=0;k<sc;k++){
        int s = starts[k];
        if (s >= nextAllowed){
            chosenStart[cc] = s;
            chosenEnd[cc] = s + m - 1;
            cc++;
            nextAllowed = s + m;
        }
    }
    // L[i]: prefix counts (i from 0..n). L[i] = number of chosen whose end <= i-1
    int *L = (int*)malloc(sizeof(int)*(n+1));
    int ptr = 0, cnt = 0;
    L[0] = 0;
    for (int i=1;i<=n;i++){
        if (ptr < cc && chosenEnd[ptr] == i-1){
            cnt++; ptr++;
        }
        L[i] = cnt;
    }

    // nextStart[i]: earliest match start >= i (or -1)
    int *nextStart = (int*)malloc(sizeof(int)*(n+1));
    for (int i=0;i<=n;i++) nextStart[i] = -1;
    int pidx = 0;
    for (int i=0;i<n;i++){
        while (pidx < sc && starts[pidx] < i) pidx++;
        nextStart[i] = (pidx < sc ? starts[pidx] : -1);
    }
    nextStart[n] = -1;

    // R[i]: suffix greedy count for T[i..n-1]
    int *R = (int*)malloc(sizeof(int)*(n+1));
    R[n] = 0;
    // We can fill from n-1 down to 0 using memoization jump
    for (int i=n-1;i>=0;i--){
        int s = nextStart[i];
        if (s == -1){
            R[i] = 0;
        } else {
            int jump = s + m;
            if (jump > n) R[i] = 1;
            else R[i] = 1 + R[jump];
        }
    }

    int total = L[n];
    long long ans = 0;
    for (int i=1;i<=n-1;i++){
        if (L[i] + R[i] == total) ans++;
    }
    printf("%lld\n", ans);

    free(T); free(P);
    free(lps); free(starts);
    free(chosenStart); free(chosenEnd);
    free(L); free(nextStart); free(R);
    return 0;
}