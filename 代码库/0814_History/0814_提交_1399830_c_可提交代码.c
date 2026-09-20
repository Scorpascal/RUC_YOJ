#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct Node {
    uint32_t key;
    int last;
    struct Node* next;
} Node;

#define BUCKETS (1u << 18) // 262144 buckets, sufficient for n<=1e5
static Node* table[BUCKETS];

static inline uint32_t hash32(uint32_t x) {
    // Knuth multiplicative hashing
    return (x * 2654435761u) & (BUCKETS - 1);
}

static Node* find_node(uint32_t key) {
    uint32_t h = hash32(key);
    Node* cur = table[h];
    while (cur) {
        if (cur->key == key) return cur;
        cur = cur->next;
    }
    return NULL;
}

static Node* insert_or_get(uint32_t key) {
    uint32_t h = hash32(key);
    Node* cur = table[h];
    while (cur) {
        if (cur->key == key) return cur;
        cur = cur->next;
    }
    Node* n = (Node*)malloc(sizeof(Node));
    n->key = key;
    n->last = 0;
    n->next = table[h];
    table[h] = n;
    return n;
}

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    uint32_t prevAns = 0;
    for (int i = 1; i <= n; ++i) {
        uint32_t x;
        if (scanf("%u", &x) != 1) return 0;
        uint32_t realId = x ^ prevAns;
        Node* node = find_node(realId);
        int ans;
        if (node == NULL) {
            // first time
            ans = i;
            node = insert_or_get(realId);
            node->last = i;
        } else {
            ans = node->last;
            node->last = i;
        }
        printf("%d\n", ans);
        prevAns = (uint32_t)ans;
    }
    return 0;
}