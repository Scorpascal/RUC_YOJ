#include <stdio.h>
#include <string.h>

#define MAX_STATE 362880
#define STATE_LEN 9

static int factorial[STATE_LEN] = {1};

static int cantor(const char *state) {
    int idx = 0;
    for (int i = 1; i < STATE_LEN; ++i) {
        factorial[i] = factorial[i - 1] * i;
    }
    for (int i = 0; i < STATE_LEN; ++i) {
        int smaller = 0;
        for (int j = i + 1; j < STATE_LEN; ++j) {
            if (state[j] < state[i]) {
                ++smaller;
            }
        }
        idx += smaller * factorial[STATE_LEN - i - 1];
    }
    return idx;
}

int main(void) {
    char start[STATE_LEN];
    char target[STATE_LEN];
    for (int i = 0; i < STATE_LEN; ++i) {
        int v;
        if (scanf("%d", &v) != 1) return 0;
        start[i] = (char)('0' + v);
    }
    for (int i = 0; i < STATE_LEN; ++i) {
        int v;
        if (scanf("%d", &v) != 1) return 0;
        target[i] = (char)('0' + v);
    }

    if (memcmp(start, target, STATE_LEN) == 0) {
        puts("0");
        return 0;
    }

    char queue[MAX_STATE][STATE_LEN];
    int depth[MAX_STATE];
    char visited[MAX_STATE] = {0};

    int front = 0, rear = 0;
    memcpy(queue[rear], start, STATE_LEN);
    depth[rear] = 0;
    visited[cantor(start)] = 1;
    ++rear;

    const int dirs[4] = {-3, 3, -1, 1};

    while (front < rear) {
        char current[STATE_LEN];
        memcpy(current, queue[front], STATE_LEN);
        int step = depth[front];
        ++front;

        int zero_pos = 0;
        while (current[zero_pos] != '0') ++zero_pos;

        for (int d = 0; d < 4; ++d) {
            int nz = zero_pos + dirs[d];
            if (nz < 0 || nz >= STATE_LEN) continue;
            if (dirs[d] == -1 && zero_pos % 3 == 0) continue;
            if (dirs[d] == 1 && zero_pos % 3 == 2) continue;

            char next_state[STATE_LEN];
            memcpy(next_state, current, STATE_LEN);
            char tmp = next_state[zero_pos];
            next_state[zero_pos] = next_state[nz];
            next_state[nz] = tmp;

            int idx = cantor(next_state);
            if (visited[idx]) continue;
            if (memcmp(next_state, target, STATE_LEN) == 0) {
                printf("%d\n", step + 1);
                return 0;
            }
            visited[idx] = 1;
            memcpy(queue[rear], next_state, STATE_LEN);
            depth[rear] = step + 1;
            ++rear;
        }
    }

    return 0;
}