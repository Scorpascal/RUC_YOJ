#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void){
    int n;
    if(scanf("%d",&n)!=1) return 0;
    int *t = (int*)malloc(sizeof(int)*n);
    if(!t) return 0;
    int maxv = 0;
    for(int i=0;i<n;i++){
        if(scanf("%d",&t[i])!=1){
            free(t);
            return 0;
        }
        if(t[i]>maxv) maxv = t[i];
    }

    int limit = (int)sqrt(maxv) + 1;
    if(limit < 2) limit = 2;

    // 埃拉托斯特尼筛法：标记合数，收集所有 <= limit 的素数
    char *is_comp = (char*)calloc(limit+1, sizeof(char)); // 0 表示可能是质数
    if(!is_comp){ free(t); return 0; }
    int *p = (int*)malloc(sizeof(int)*(limit+1));
    if(!p){ free(t); free(is_comp); return 0; }
    int a = 0;
    for(int i = 2; i <= limit; i++){
        if(!is_comp[i]){
            p[a++] = i;
            long long start = (long long)i * i;
            for(long long j = start; j <= limit; j += i) is_comp[j] = 1;
        }
    }

    // 用筛出的素数检测每个输入
    for(int i=0;i<n;i++){
        int x = t[i];
        if(x <= 1){ printf("NO "); continue; }
        int lim = (int)sqrt(x);
        int is_prime = 1;
        for(int j=0; j<a && p[j] <= lim; j++){
            if(x % p[j] == 0){ is_prime = 0; break; }
        }
        printf(is_prime ? "YES " : "NO ");
    }
    printf("\n");

    free(t);
    free(is_comp);
    free(p);
    return 0;
}//GPT5mini