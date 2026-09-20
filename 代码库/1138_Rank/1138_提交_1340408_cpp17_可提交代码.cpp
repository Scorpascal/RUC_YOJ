#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

struct Entry {
    long long score;
    long long timestamp;
    std::string name;
};

bool before(const Entry& left, const Entry& right) {
    if (left.score != right.score) return left.score > right.score;
    return left.timestamp < right.timestamp;
}

struct Node {
    Entry entry;
    std::uint32_t priority;
    int left = 0;
    int right = 0;
    int size = 1;
};

std::vector<Node> nodes(1);
std::uint32_t random_state = 0x243f6a88u;

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

void split(int root, const Entry& key, int& left, int& right) {
    if (root == 0) {
        left = right = 0;
        return;
    }
    if (before(nodes[root].entry, key)) {
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
        split(root, nodes[inserted].entry, nodes[inserted].left, nodes[inserted].right);
        pull(inserted);
        return inserted;
    }
    if (before(nodes[inserted].entry, nodes[root].entry)) {
        nodes[root].left = insert_node(nodes[root].left, inserted);
    } else {
        nodes[root].right = insert_node(nodes[root].right, inserted);
    }
    pull(root);
    return root;
}

bool equal_entry(const Entry& left, const Entry& right) {
    return !before(left, right) && !before(right, left);
}

int erase_node(int root, const Entry& key) {
    if (root == 0) return 0;
    if (equal_entry(nodes[root].entry, key)) {
        return merge_trees(nodes[root].left, nodes[root].right);
    }
    if (before(key, nodes[root].entry)) {
        nodes[root].left = erase_node(nodes[root].left, key);
    } else {
        nodes[root].right = erase_node(nodes[root].right, key);
    }
    pull(root);
    return root;
}

int order_of_key(int root, const Entry& key) {
    if (root == 0) return 0;
    if (before(nodes[root].entry, key)) {
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

int make_node(const Entry& entry) {
    nodes.push_back(Node{entry, next_priority()});
    return static_cast<int>(nodes.size()) - 1;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int request_count;
    if (!(std::cin >> request_count)) return 0;
    nodes.reserve(static_cast<std::size_t>(request_count) + 1);

    int root = 0;
    long long timestamp = 0;
    std::unordered_map<std::string, Entry> current;
    current.reserve(static_cast<std::size_t>(request_count) * 2 + 1);

    for (int i = 0; i < request_count; ++i) {
        char operation;
        std::cin >> operation;
        if (operation == '+') {
            std::string name;
            long long score;
            std::cin >> name >> score;
            auto old = current.find(name);
            if (old != current.end()) root = erase_node(root, old->second);
            Entry entry{score, timestamp++, name};
            root = insert_node(root, make_node(entry));
            current[name] = entry;
        } else if (operation == '?') {
            std::string query;
            std::cin >> query;
            if (!query.empty() && std::isdigit(static_cast<unsigned char>(query[0]))) {
                int start = std::stoi(query);
                for (int offset = 0; offset < 10 && start + offset <= size_of(root); ++offset) {
                    if (offset != 0) std::cout << ' ';
                    std::cout << nodes[kth(root, start + offset)].entry.name;
                }
                std::cout << '\n';
            } else {
                const Entry& entry = current.find(query)->second;
                std::cout << order_of_key(root, entry) + 1 << '\n';
            }
        }
    }
    return 0;
}
