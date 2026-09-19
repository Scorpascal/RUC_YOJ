#include <stdio.h>
#include <stdlib.h>

static inline int fastRead() {
    int c = getchar();
    while (c != EOF && c <= ' ') c = getchar();
    int x = 0;
    while (c >= '0' && c <= '9') {
        x = x * 10 + (c - '0');
        c = getchar();
    }
    return x;
}

int main() {
    int m = fastRead();
    int n = fastRead();
    int total = m * n;

    unsigned char *dir = (unsigned char*)malloc(total);
    int *next = (int*)malloc(sizeof(int) * total);
    int *reach = (int*)malloc(sizeof(int) * total);
    unsigned char *state = (unsigned char*)calloc(total, 1); // 0 未访 1 栈中 2 完成
    int *stack = (int*)malloc(sizeof(int) * total);

    // 读入方向 (y 行, x 列)
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < m; ++x) {
            dir[y * m + x] = (unsigned char)fastRead();
        }
    }

    // 构建 next 数组 (功能图)
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < m; ++x) {
            int idx = y * m + x;
            int d = dir[idx];
            int nx = x, ny = y;
            switch (d) {
                case 1: nx = x - 1; break;
                case 2: nx = x + 1; break;
                case 3: ny = y - 1; break;
                case 4: ny = y + 1; break;
                default: nx = -1; ny = -1; break;
            }
            if (nx >= 0 && nx < m && ny >= 0 && ny < n) {
                next[idx] = ny * m + nx;
            } else {
                next[idx] = -1;
            }
        }
    }

    int maxReach = 0;

    for (int start = 0; start < total; ++start) {
        if (state[start] != 0) continue;

        int top = 0;
        int u = start;

        // 前向遍历
        while (1) {
            state[u] = 1;
            stack[top++] = u;
            int v = next[u];

            if (v == -1) { // 终止
                reach[u] = 1;
                state[u] = 2;
                break;
            }
            if (state[v] == 0) { // 继续
                u = v;
                continue;
            }
            if (state[v] == 1) { // 环
                // 找到环起点在栈中的位置
                int pos = top - 1;
                while (pos >= 0 && stack[pos] != v) --pos;
                int cycleLen = top - pos;
                for (int k = pos; k < top; ++k) {
                    int node = stack[k];
                    reach[node] = cycleLen;
                    state[node] = 2;
                }
                break;
            }
            if (state[v] == 2) { // 后继已知
                reach[u] = reach[v] + 1;
                state[u] = 2;
                break;
            }
        }

        // 回溯填充链上剩余节点
        for (int k = top - 2; k >= 0; --k) {
            int node = stack[k];
            if (state[node] == 2) continue;
            int v = next[node];
            if (v == -1) {
                reach[node] = 1;
            } else {
                reach[node] = reach[v] + 1;
            }
            state[node] = 2;
        }

        // 更新最大
        for (int k = 0; k < top; ++k) {
            if (reach[stack[k]] > maxReach) {
                maxReach = reach[stack[k]];
            }
        }
    }

    // 按 x 再 y 输出
    for (int x = 0; x < m; ++x) {
        for (int y = 0; y < n; ++y) {
            int idx = y * m + x;
            if (reach[idx] == maxReach) {
                printf("%d %d\n", x, y);
            }
        }
    }

    free(dir);
    free(next);
    free(reach);
    free(state);
    free(stack);
    return 0;
}