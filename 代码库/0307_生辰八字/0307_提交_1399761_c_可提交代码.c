// ...existing code...
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

typedef struct {
    long long id;
    double sim;
} Candidate;

int cmp(const void *a, const void *b) {
    const Candidate *x = (const Candidate *)a;
    const Candidate *y = (const Candidate *)b;
    const double eps = 1e-10;
    if (fabs(x->sim - y->sim) > eps) {
        return (x->sim > y->sim) ? -1 : 1; // descending by similarity
    } else {
        if (x->id != y->id) return (x->id > y->id) ? -1 : 1; // descending by id
        return 0;
    }
}

int main(void) {
    int n, k;
    if (scanf("%d %d", &n, &k) != 2) return 0;

    int A[8];
    for (int i = 0; i < 8; ++i) scanf("%d", &A[i]);

    double normA2 = 0.0;
    for (int i = 0; i < 8; ++i) normA2 += (double)A[i] * (double)A[i];
    double normA = sqrt(normA2);

    Candidate *arr = (Candidate *)malloc(sizeof(Candidate) * n);
    if (!arr) return 0;

    for (int i = 0; i < n; ++i) {
        long long id;
        int B[8];
        if (scanf("%lld", &id) != 1) { id = 0; }
        for (int j = 0; j < 8; ++j) {
            if (scanf("%d", &B[j]) != 1) B[j] = 0;
        }

        double dot = 0.0, normB2 = 0.0;
        for (int j = 0; j < 8; ++j) {
            dot += (double)A[j] * (double)B[j];
            normB2 += (double)B[j] * (double)B[j];
        }
        double normB = sqrt(normB2);
        double sim = (normA > 0.0 && normB > 0.0) ? (dot / (normA * normB)) : 0.0;

        arr[i].id = id;
        arr[i].sim = sim;
    }

    qsort(arr, n, sizeof(Candidate), cmp);

    for (int i = 0; i < k; ++i) {
        if (i) printf(" ");
        printf("%lld", arr[i].id);
    }
    printf("\n");

    free(arr);
    return 0;
}
// ...existing code...