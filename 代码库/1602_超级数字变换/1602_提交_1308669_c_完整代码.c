#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_L 200005
#define INF LLONG_MAX

typedef long long ll;

typedef struct {
    int value;
    ll cost;
} Node;

typedef struct {
    int from;       // 前驱节点
    int operation;  // 使用的操作：1(+1), 2(×2), 3(×3), 4(-1)
} Path;

ll dist[MAX_L];
Path path[MAX_L];
int visited[MAX_L];

// 最小堆
Node heap[MAX_L * 4];
int heap_size = 0;

void swap(Node *a, Node *b) {
    Node temp = *a;
    *a = *b;
    *b = temp;
}

void heap_push(int value, ll cost) {
    heap_size++;
    int i = heap_size;
    heap[i] = (Node){value, cost};
    
    while (i > 1 && heap[i].cost < heap[i/2].cost) {
        swap(&heap[i], &heap[i/2]);
        i = i/2;
    }
}

Node heap_pop() {
    Node result = heap[1];
    heap[1] = heap[heap_size];
    heap_size--;
    
    int i = 1;
    while (i * 2 <= heap_size) {
        int child = i * 2;
        if (child + 1 <= heap_size && heap[child + 1].cost < heap[child].cost) {
            child++;
        }
        if (heap[child].cost < heap[i].cost) {
            swap(&heap[child], &heap[i]);
            i = child;
        } else {
            break;
        }
    }
    
    return result;
}

void dijkstra(int n, int m, int a, int b, int c, int d, int L) {
    // 初始化
    for (int i = 0; i <= L; i++) {
        dist[i] = INF;
        visited[i] = 0;
        path[i] = (Path){-1, 0};
    }
    
    dist[n] = 0;
    heap_push(n, 0);
    
    while (heap_size > 0) {
        Node current = heap_pop();
        int u = current.value;
        
        if (visited[u]) continue;
        visited[u] = 1;
        
        if (u == m) break;
        
        // 操作1: u + 1
        if (u + 1 <= L) {
            ll new_cost = dist[u] + a;
            if (new_cost < dist[u + 1]) {
                dist[u + 1] = new_cost;
                path[u + 1] = (Path){u, 1};
                heap_push(u + 1, new_cost);
            }
        }
        
        // 操作2: u × 2
        if (u * 2 <= L) {
            ll new_cost = dist[u] + b;
            if (new_cost < dist[u * 2]) {
                dist[u * 2] = new_cost;
                path[u * 2] = (Path){u, 2};
                heap_push(u * 2, new_cost);
            }
        }
        
        // 操作3: u × 3
        if (u * 3 <= L) {
            ll new_cost = dist[u] + c;
            if (new_cost < dist[u * 3]) {
                dist[u * 3] = new_cost;
                path[u * 3] = (Path){u, 3};
                heap_push(u * 3, new_cost);
            }
        }
        
        // 操作4: u - 1
        if (u - 1 >= 0) {
            ll new_cost = dist[u] + d;
            if (new_cost < dist[u - 1]) {
                dist[u - 1] = new_cost;
                path[u - 1] = (Path){u, 4};
                heap_push(u - 1, new_cost);
            }
        }
    }
}

void print_path(int n, int m) {
    if (dist[m] == INF) {
        printf("-1\n");
        return;
    }
    
    // 反向追踪路径
    int steps[MAX_L];
    int step_count = 0;
    int current = m;
    
    while (current != n) {
        steps[step_count++] = path[current].operation;
        current = path[current].from;
    }
    
    // 输出结果
    printf("%lld\n", dist[m]);
    printf("%d\n", step_count);
    
    for (int i = step_count - 1; i >= 0; i--) {
        switch (steps[i]) {
            case 1: printf("+1\n"); break;
            case 2: printf("x2\n"); break;
            case 3: printf("x3\n"); break;
            case 4: printf("-1\n"); break;
        }
    }
}

int main() {
    int n, m, a, b, c, d, L;
    scanf("%d %d", &n, &m);
    scanf("%d %d %d %d", &a, &b, &c, &d);
    scanf("%d", &L);
    
    dijkstra(n, m, a, b, c, d, L);
    print_path(n, m);
    
    return 0;
}