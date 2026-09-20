#include <cstdint>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

struct Key {
    int value;
    int serial;
};

bool less_key(const Key& left, const Key& right) {
    if (left.value != right.value) return left.value < right.value;
    return left.serial < right.serial;
}

struct Node {
    Key key;
    std::uint32_t priority;
    int left = 0;
    int right = 0;
    int size = 1;
};

std::vector<Node> nodes(1);
std::uint32_t random_state = 0x9e3779b9u;

std::uint32_t next_priority() {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

int size_of(int root) {
    return root == 0 ? 0 : nodes[root].size;
}

void pull(int root) {
    if (root != 0) {
        nodes[root].size = 1 + size_of(nodes[root].left) + size_of(nodes[root].right);
    }
}

void split(int root, const Key& key, int& left, int& right) {
    if (root == 0) {
        left = right = 0;
        return;
    }
    if (less_key(nodes[root].key, key)) {
        left = root;
        split(nodes[root].right, key, nodes[root].right, right);
        pull(left);
    } else {
        right = root;
        split(nodes[root].left, key, left, nodes[root].left);
        pull(right);
    }
}

int merge_trees(int left, int right) {
    if (left == 0) return right;
    if (right == 0) return left;
    if (nodes[left].priority > nodes[right].priority) {
        nodes[left].right = merge_trees(nodes[left].right, right);
        pull(left);
        return left;
    }
    nodes[right].left = merge_trees(left, nodes[right].left);
    pull(right);
    return right;
}

int insert_node(int root, int inserted) {
    if (root == 0) return inserted;
    if (nodes[inserted].priority > nodes[root].priority) {
        split(root, nodes[inserted].key, nodes[inserted].left, nodes[inserted].right);
        pull(inserted);
        return inserted;
    }
    if (less_key(nodes[inserted].key, nodes[root].key)) {
        nodes[root].left = insert_node(nodes[root].left, inserted);
    } else {
        nodes[root].right = insert_node(nodes[root].right, inserted);
    }
    pull(root);
    return root;
}

int erase_node(int root, const Key& key) {
    if (root == 0) return 0;
    if (!less_key(nodes[root].key, key) && !less_key(key, nodes[root].key)) {
        return merge_trees(nodes[root].left, nodes[root].right);
    }
    if (less_key(key, nodes[root].key)) {
        nodes[root].left = erase_node(nodes[root].left, key);
    } else {
        nodes[root].right = erase_node(nodes[root].right, key);
    }
    pull(root);
    return root;
}

int order_of_key(int root, const Key& key) {
    if (root == 0) return 0;
    if (less_key(nodes[root].key, key)) {
        return size_of(nodes[root].left) + 1 + order_of_key(nodes[root].right, key);
    }
    return order_of_key(nodes[root].left, key);
}

int kth(int root, int rank) {
    while (root != 0) {
        int left_size = size_of(nodes[root].left);
        if (rank == left_size + 1) return root;
        if (rank <= left_size) {
            root = nodes[root].left;
        } else {
            rank -= left_size + 1;
            root = nodes[root].right;
        }
    }
    return 0;
}

int make_node(const Key& key) {
    nodes.push_back(Node{key, next_priority()});
    return static_cast<int>(nodes.size()) - 1;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int operation_count;
    if (!(std::cin >> operation_count)) return 0;
    nodes.reserve(static_cast<std::size_t>(operation_count) + 1);

    int root = 0;
    int serial = 0;
    const int minimum_serial = std::numeric_limits<int>::min();
    const int maximum_serial = std::numeric_limits<int>::max();

    for (int i = 0; i < operation_count; ++i) {
        int operation;
        int value;
        std::cin >> operation >> value;
        if (operation == 1) {
            root = insert_node(root, make_node({value, serial++}));
        } else if (operation == 2) {
            int position = order_of_key(root, {value, minimum_serial});
            if (position < size_of(root)) {
                int node = kth(root, position + 1);
                if (nodes[node].key.value == value) root = erase_node(root, nodes[node].key);
            }
        } else if (operation == 3) {
            std::cout << order_of_key(root, {value, minimum_serial}) + 1 << '\n';
        } else if (operation == 4) {
            std::cout << nodes[kth(root, value)].key.value << '\n';
        } else if (operation == 5) {
            int position = order_of_key(root, {value, minimum_serial});
            std::cout << nodes[kth(root, position)].key.value << '\n';
        } else if (operation == 6) {
            int position = order_of_key(root, {value, maximum_serial});
            std::cout << nodes[kth(root, position + 1)].key.value << '\n';
        }
    }
    return 0;
}
