#include <stdio.h>
#include <stdlib.h>

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    int *T = (int*)malloc((n+1)*sizeof(int));
    for (int i = 1; i <= n; ++i) {
        scanf("%d", &T[i]);
    }

    // 0: unvisited, 1: visiting, 2: done
    char *state = (char*)calloc(n+1, sizeof(char));
    int *step_in = (int*)malloc((n+1)*sizeof(int)); // step when node entered (only valid for visiting)

    int ans = n; // upper bound
    for (int i = 1; i <= n; ++i) {
        if (state[i] != 0) continue;
        int u = i;
        int step = 0;
        // Use iterative walk with marking; also record a walk id by reusing step_in with step values
        while (state[u] == 0) {
            state[u] = 1;
            step_in[u] = step++;
            u = T[u];
        }
        if (state[u] == 1) {
            // Found a cycle; cycle length = current step - step_in[u]
            int cycle_len = step - step_in[u];
            if (cycle_len < ans) ans = cycle_len;
        }
        // Mark all nodes in this chain as done
        // We need to walk again from i to mark until reaching a node already done
        u = i;
        while (state[u] == 1) {
            state[u] = 2;
            u = T[u];
        }
    }

    printf("%d\n", ans);

    free(T);
    free(state);
    free(step_in);
    return 0;
}