#include <stdio.h>
#include <stdlib.h>

typedef struct { int id; int score; int rank; } Student;

int cmp(const void *a, const void *b) {
    const Student *A = a, *B = b;
    if (A->score != B->score) return B->score - A->score; // score 降序
    return A->id - B->id; // id 升序
}

int main(void) {
    int N;
    if (scanf("%d", &N) != 1) return 0;
    Student *s = malloc(sizeof(Student) * N);
    for (int i = 0; i < N; ++i) {
        scanf("%d %d", &s[i].id, &s[i].score);
        s[i].rank = 0;
    }

    qsort(s, N, sizeof(Student), cmp);

    int prevScore = -1, prevRank = 0;
    for (int i = 0; i < N; ++i) {
        if (i == 0 || s[i].score != prevScore) {
            prevRank = i + 1;
            prevScore = s[i].score;
        }
        s[i].rank = prevRank;
    }

    for (int i = 0; i < N; ++i) {
        printf("%d %d %d", s[i].rank, s[i].id, s[i].score);
        if (i != N - 1) putchar('\n');
    }

    free(s);
    return 0;
}