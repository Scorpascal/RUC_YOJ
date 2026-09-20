#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    long long freq;
    int ch;            // 0..25 for 'a'..'z', or -1 for internal
    struct Node *left, *right;
} Node;

typedef struct {
    Node **data;
    int size, cap;
} MinHeap;

Node* new_node(long long freq, int ch, Node* l, Node* r) {
    Node* n = (Node*)malloc(sizeof(Node));
    n->freq = freq;
    n->ch = ch;
    n->left = l;
    n->right = r;
    return n;
}

MinHeap* heap_new(int cap) {
    MinHeap* h = (MinHeap*)malloc(sizeof(MinHeap));
    h->data = (Node**)malloc(sizeof(Node*) * (cap + 1));
    h->size = 0;
    h->cap = cap;
    return h;
}

void heap_swap(Node** a, Node** b) {
    Node* t = *a; *a = *b; *b = t;
}

void heap_push(MinHeap* h, Node* n) {
    int i = ++h->size;
    h->data[i] = n;
    while (i > 1) {
        int p = i / 2;
        if (h->data[p]->freq <= h->data[i]->freq) break;
        heap_swap(&h->data[p], &h->data[i]);
        i = p;
    }
}

Node* heap_pop(MinHeap* h) {
    if (h->size == 0) return NULL;
    Node* top = h->data[1];
    h->data[1] = h->data[h->size--];
    int i = 1;
    while (1) {
        int l = i * 2, r = l + 1, smallest = i;
        if (l <= h->size && h->data[l]->freq < h->data[smallest]->freq) smallest = l;
        if (r <= h->size && h->data[r]->freq < h->data[smallest]->freq) smallest = r;
        if (smallest == i) break;
        heap_swap(&h->data[i], &h->data[smallest]);
        i = smallest;
    }
    return top;
}

void gen_codes(Node* root, char* buf, int depth, char codes[26][128], long long freq[26], long long* total) {
    if (!root) return;
    if (!root->left && !root->right) {
        // leaf
        if (depth == 0) { // single character case
            buf[0] = '0';
            depth = 1;
        }
        buf[depth] = '\0';
        if (root->ch >= 0) {
            strcpy(codes[root->ch], buf);
            *total += (long long)depth * freq[root->ch];
        }
        return;
    }
    if (root->left) {
        buf[depth] = '0';
        gen_codes(root->left, buf, depth + 1, codes, freq, total);
    }
    if (root->right) {
        buf[depth] = '1';
        gen_codes(root->right, buf, depth + 1, codes, freq, total);
    }
}

int main() {
    // Read input line (lowercase letters only)
    // n up to 500,000 -> use dynamic buffer
    char *s = NULL;
    size_t cap = 0;
    ssize_t len = getline(&s, &cap, stdin);
    if (len <= 0) return 0;
    // strip newline if present
    if (len > 0 && s[len - 1] == '\n') s[--len] = '\0';

    long long freq[26] = {0};
    for (ssize_t i = 0; i < len; ++i) {
        char c = s[i];
        if (c >= 'a' && c <= 'z') freq[c - 'a']++;
        else {
            // per problem, input is lowercase; ignore otherwise
        }
    }

    // Build heap with present characters
    int distinct = 0;
    for (int i = 0; i < 26; ++i) if (freq[i] > 0) distinct++;

    if (distinct == 0) {
        // Empty input: total length 0, no codes required
        printf("0\n");
        free(s);
        return 0;
    }

    MinHeap* h = heap_new(26 * 2);
    for (int i = 0; i < 26; ++i) {
        if (freq[i] > 0) heap_push(h, new_node(freq[i], i, NULL, NULL));
    }

    // If only one symbol, root is that node; else combine
    if (h->size >= 2) {
        while (h->size > 1) {
            Node* a = heap_pop(h);
            Node* b = heap_pop(h);
            Node* parent = new_node(a->freq + b->freq, -1, a, b);
            heap_push(h, parent);
        }
    }
    Node* root = heap_pop(h);

    // Generate codes
    char codes[26][128];
    for (int i = 0; i < 26; ++i) codes[i][0] = '\0';
    char buf[128];
    long long total = 0;
    gen_codes(root, buf, 0, codes, freq, &total);

    // Output total and codes for appeared chars
    printf("%lld\n", total);
    for (int i = 0; i < 26; ++i) {
        if (freq[i] > 0) {
            printf("%c %s\n", 'a' + i, codes[i]);
        }
    }

    // Cleanup
    // Free tree nodes via post-order traversal
    // Optional: omit to simplify since program exits
    free(h->data);
    free(h);
    free(s);
    return 0;
}