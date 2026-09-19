#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int s, f;
} Activity;

int cmp(const void *a, const void *b) {
    Activity *x = (Activity *)a;
    Activity *y = (Activity *)b;
    return x->f - y->f;
}

int main() {
    int n;
    scanf("%d", &n);
    Activity acts[1005];
    for (int i = 0; i < n; i++) {
        scanf("%d %d", &acts[i].s, &acts[i].f);
    }
    qsort(acts, n, sizeof(Activity), cmp);

    int cnt = 0, last_end = -1;
    for (int i = 0; i < n; i++) {
        if (acts[i].s >= last_end) {
            cnt++;
            last_end = acts[i].f;
        }
    }
    printf("%d\n", cnt);
    return 0;
}