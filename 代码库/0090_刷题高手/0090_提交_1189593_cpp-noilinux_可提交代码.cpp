#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int sno;
    int cnt;
} Student;

int cmp_int(const void *a, const void *b) {
    int A = *(const int*)a, B = *(const int*)b;
    return (A > B) - (A < B);
}

int cmp_student(const void *a, const void *b) {
    const Student *A = (const Student*)a;
    const Student *B = (const Student*)b;
    if (A->cnt != B->cnt) return B->cnt - A->cnt; // 数量降序
    return A->sno - B->sno; // 学号升序
}

// 返回下标，未找到返回 -1
int bin_find(const int *arr, int n, int x) {
    int l = 0, r = n - 1;
    while (l <= r) {
        int m = (l + r) >> 1;
        if (arr[m] == x) return m;
        if (arr[m] < x) l = m + 1;
        else r = m - 1;
    }
    return -1;
}

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    int teacher_raw[105];
    for (int i = 0; i < n; ++i) scanf("%d", &teacher_raw[i]);

    // 老师题目排序，统计为(题号,需求次数)的多重集
    qsort(teacher_raw, n, sizeof(int), cmp_int);
    int teach_val[105], teach_need[105], tn = 0;
    for (int i = 0; i < n; ++i) {
        if (i == 0 || teacher_raw[i] != teacher_raw[i-1]) {
            teach_val[tn] = teacher_raw[i];
            teach_need[tn] = 1;
            tn++;
        } else {
            teach_need[tn-1]++;
        }
    }

    int m, k;
    if (scanf("%d %d", &m, &k) != 2) return 0;

    Student students[105];
    for (int i = 0; i < m; ++i) {
        int sno, p;
        scanf("%d %d", &sno, &p);
        int used[105] = {0}; // 对该同学，消耗老师需求次数
        int cnt = 0;
        for (int j = 0; j < p; ++j) {
            int prob;
            scanf("%d", &prob);
            int idx = bin_find(teach_val, tn, prob);
            if (idx >= 0 && used[idx] < teach_need[idx]) {
                used[idx]++; // 消耗一次需求
                cnt++;
            }
        }
        students[i].sno = sno;
        students[i].cnt = cnt;
    }

    qsort(students, m, sizeof(Student), cmp_student);

    // 按“成绩分组”输出前 k 组（dense ranking）
    int groups = 0;
    int first = 1;
    for (int i = 0; i < m; ++i) {
        if (i == 0 || students[i].cnt != students[i-1].cnt) {
            groups++;
        }
        if (groups > k) break;
        if (!first) printf(" ");
        printf("%d", students[i].sno);
        first = 0;
    }
    printf("\n");
    return 0;
}