#include <stdio.h>
#include <stdlib.h>

int findMax(int *p, int nSize)
{
    int maxIdx = 0;
    for (int i = 1; i < nSize; i++) {
        if (p[i] > p[maxIdx])
            maxIdx = i;
    }
    // 交换最大值到首元素
    int tmp = p[0];
    p[0] = p[maxIdx];
    p[maxIdx] = tmp;
    return p[0];
}

int main()
{
    int ary[2100] = {3, 2, 1, 5, 6, 7, 9, 10}, n = 8, nMax, i;
    scanf("%d", &n);
    for (i = 0; i < n; i++)
        scanf("%d", &ary[i]);

    nMax = findMax(ary, n);

    for (i = 0; i < n; i++)
        printf("%d ", ary[i]);
    printf("\n");

    printf("%d\n", nMax);
    return 0;
}