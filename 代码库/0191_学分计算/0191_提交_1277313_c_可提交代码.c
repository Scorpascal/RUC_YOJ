#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int id;
    int credit;
} Student;

int cmp(const void *a, const void *b) {
    const Student *A = a;
    const Student *B = b;
    if (B->credit != A->credit) return B->credit - A->credit; // 学分降序
    return A->id - B->id; // 学号升序
}

int main() {
    int N, M;
    if (scanf("%d %d", &N, &M) != 2) return 0;
    int credits[10];
    for (int i = 0; i < M; ++i) scanf("%d", &credits[i]);

    Student *arr = malloc(sizeof(Student) * N);
    for (int i = 0; i < N; ++i) {
        int id;
        scanf("%d", &id);
        int sum = 0;
        for (int j = 0; j < M; ++j) {
            int score;
            scanf("%d", &score);
            if (score >= 60) sum += credits[j];
        }
        arr[i].id = id;
        arr[i].credit = sum;
    }

    qsort(arr, N, sizeof(Student), cmp);
    for (int i = 0; i < N; ++i) {
        printf("%d %d\n", arr[i].id, arr[i].credit);
    }

    free(arr);
    return 0;
}