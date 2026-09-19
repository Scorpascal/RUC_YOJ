#!/usr/bin/env python3
"""Generate explicit C++17 candidates for the two captured PBDS solutions.

The raw submissions remain untouched.  These replacements use a randomized
treap with subtree sizes, so they are suitable for the macOS Clang toolchain
without GNU ``ext/pb_ds``.  They are still candidates: this script does not
claim online equivalence or change public readiness.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
RAW_ROOT = ROOT / "代码库"
DATA_PATH = ROOT / "data" / "problems.json"
OUTPUT_ROOT = ROOT / "staging" / "cpp17-portable"
REPORT_PATH = ROOT / "staging" / "cpp17-porting-report.json"


def sha256(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def balanced_tree_solution() -> str:
    return r'''#include <cstdint>
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
'''


def ranking_solution() -> str:
    return r'''#include <cctype>
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
'''


SOLUTIONS = {"1009": balanced_tree_solution, "1138": ranking_solution}


def main() -> int:
    parser = argparse.ArgumentParser(description="生成不依赖 GNU PBDS 的 C++17 候选")
    parser.add_argument("--check", action="store_true", help="只检查已有输出，不重写候选")
    args = parser.parse_args()

    records = json.loads(DATA_PATH.read_text(encoding="utf-8"))["records"]
    generated: list[dict[str, Any]] = []
    missing: list[str] = []
    for record in records:
        problem_no = str(record["problemNo"])
        factory = SOLUTIONS.get(problem_no)
        if factory is None:
            continue
        source = factory()
        for key in ("completeCode", "directlySubmittableCode"):
            raw_path = ROOT / str(record["archive"][key])
            output_path = OUTPUT_ROOT / raw_path.relative_to(RAW_ROOT)
            if not args.check:
                output_path.parent.mkdir(parents=True, exist_ok=True)
                output_path.write_text(source, encoding="utf-8", newline="\n")
            if not output_path.is_file():
                missing.append(output_path.relative_to(ROOT).as_posix())
            generated.append(
                {
                    "problemNo": problem_no,
                    "source": raw_path.relative_to(ROOT).as_posix(),
                    "candidate": output_path.relative_to(ROOT).as_posix(),
                    "candidateSha256": sha256(source),
                }
            )

    report = {
        "schemaVersion": 1,
        "purpose": "candidate-only C++17 replacements for ext/pb_ds submissions",
        "rawArchiveChanged": False,
        "problems": sorted(SOLUTIONS),
        "files": generated,
        "missing": missing,
        "notes": [
            "使用带子树大小的随机 Treap 替代 GNU PBDS 顺序统计树。",
            "候选尚未经过 YOJ 在线回收源码比对，因此不改变 RAW_CAPTURED 或 publish 状态。",
        ],
    }
    if not args.check:
        REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
        REPORT_PATH.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"problems": sorted(SOLUTIONS), "files": len(generated), "missing": len(missing)}, ensure_ascii=False, indent=2))
    return 0 if not missing else 1


if __name__ == "__main__":
    raise SystemExit(main())
