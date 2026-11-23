#include <algorithm>
#include <cstddef>
#include <deque>
#include <iostream>
#include <vector>

struct BTree {
    using Key = long long;

    struct Page {
        std::vector<Key> keys;
        std::vector<int> children;

        std::size_t position(Key key) const {
            return std::lower_bound(keys.begin(), keys.end(), key) - keys.begin();
        }
        bool matches(std::size_t i, Key key) const {
            return i < keys.size() && keys[i] == key;
        }
    };
    struct Promotion {
        Key key = 0;
        int right = 0;
    };

    const std::size_t order, minKeys;
    std::deque<Page> pages{1};
    std::vector<int> unusedPages;
    int root = 0, height = 0;
    std::size_t keyCount = 0;

    explicit BTree(std::size_t m) : order(m), minKeys((m - 1) / 2) {}

    int newPage() {
        if (!unusedPages.empty()) {
            int id = unusedPages.back();
            unusedPages.pop_back();
            return id;
        }
        pages.emplace_back();
        return static_cast<int>(pages.size()) - 1;
    }
    void releasePage(int id) {
        pages[id] = Page{};
        unusedPages.push_back(id);
    }

    int insert(Key key) {
        if (!root) {
            root = newPage();
            pages[root].keys.push_back(key);
            height = 1;
            keyCount = 1;
            return 0;
        }
        int splits = 0;
        Promotion result = insertBelow(root, key, splits);
        if (result.right) {
            int previousRoot = root;
            root = newPage();
            pages[root].keys.push_back(result.key);
            pages[root].children = {previousRoot, result.right};
            ++height;
        }
        return splits;
    }

    Promotion insertBelow(int id, Key key, int &splits) {
        Page &page = pages[id];
        const std::size_t pos = page.position(key);
        if (page.matches(pos, key)) return {};
        if (page.children.empty()) {
            page.keys.insert(page.keys.begin() + pos, key);
            ++keyCount;
        } else {
            Promotion child = insertBelow(page.children[pos], key, splits);
            if (!child.right) return {};
            page.keys.insert(page.keys.begin() + pos, child.key);
            page.children.insert(page.children.begin() + pos + 1, child.right);
        }
        if (page.keys.size() < order) return {};

        // Splitting an overfull page promotes the right middle key for even M.
        const std::size_t middle = page.keys.size() / 2;
        const Key separator = page.keys[middle];
        const int right = newPage();
        pages[right].keys.assign(page.keys.begin() + middle + 1, page.keys.end());
        page.keys.resize(middle);
        if (!page.children.empty()) {
            auto first = page.children.begin() + middle + 1;
            pages[right].children.assign(first, page.children.end());
            page.children.resize(middle + 1);
        }
        ++splits;
        return {separator, right};
    }

    void erase(Key key) {
        if (!root || !eraseBelow(root, key)) return;
        --keyCount;
        if (!pages[root].keys.empty()) return;
        const int previousRoot = root;
        root = pages[root].children.empty() ? 0 : pages[root].children.front();
        --height;
        releasePage(previousRoot);
    }

    bool eraseBelow(int id, Key key) {
        Page &page = pages[id];
        const std::size_t pos = page.position(key);
        if (page.children.empty()) {
            if (!page.matches(pos, key)) return false;
            page.keys.erase(page.keys.begin() + pos);
            return true;
        }
        if (page.matches(pos, key)) {
            page.keys[pos] = takeLargest(page.children[pos]);
        } else if (!eraseBelow(page.children[pos], key)) {
            return false;
        }
        restoreChild(id, pos);
        return true;
    }

    Key takeLargest(int id) {
        Page &page = pages[id];
        if (page.children.empty()) {
            const Key result = page.keys.back();
            page.keys.pop_back();
            return result;
        }
        const std::size_t pos = page.children.size() - 1;
        const Key result = takeLargest(page.children[pos]);
        restoreChild(id, pos);
        return result;
    }

    void restoreChild(int parentId, std::size_t pos) {
        Page &parent = pages[parentId];
        Page &target = pages[parent.children[pos]];
        if (target.keys.size() >= minKeys) return;
        if (pos > 0) {
            Page &left = pages[parent.children[pos - 1]];
            if (left.keys.size() > minKeys) {
                target.keys.insert(target.keys.begin(), parent.keys[pos - 1]);
                parent.keys[pos - 1] = left.keys.back();
                left.keys.pop_back();
                if (!left.children.empty()) {
                    target.children.insert(target.children.begin(), left.children.back());
                    left.children.pop_back();
                }
                return;
            }
        }
        if (pos + 1 < parent.children.size()) {
            Page &right = pages[parent.children[pos + 1]];
            if (right.keys.size() > minKeys) {
                target.keys.push_back(parent.keys[pos]);
                parent.keys[pos] = right.keys.front();
                right.keys.erase(right.keys.begin());
                if (!right.children.empty()) {
                    target.children.push_back(right.children.front());
                    right.children.erase(right.children.begin());
                }
                return;
            }
        }
        mergeChildren(parentId, pos > 0 ? pos - 1 : pos);
    }

    void mergeChildren(int parentId, std::size_t pos) {
        Page &parent = pages[parentId];
        const int rightId = parent.children[pos + 1];
        Page &left = pages[parent.children[pos]];
        Page &right = pages[rightId];
        left.keys.push_back(parent.keys[pos]);
        left.keys.insert(left.keys.end(), right.keys.begin(), right.keys.end());
        left.children.insert(left.children.end(), right.children.begin(), right.children.end());
        parent.keys.erase(parent.keys.begin() + pos);
        parent.children.erase(parent.children.begin() + pos + 1);
        releasePage(rightId);
    }

    template<class Visitor>
    void search(Key key, Visitor visit) const {
        for (int id = root; id; ) {
            visit(id);
            const Page &page = pages[id];
            const std::size_t pos = page.position(key);
            if (page.matches(pos, key) || page.children.empty()) return;
            id = page.children[pos];
        }
    }

    template<class Visitor>
    void visitKeys(int id, Visitor &visit) const {
        if (!id) return;
        const Page &page = pages[id];
        for (std::size_t i = 0; i < page.keys.size(); ++i) {
            if (!page.children.empty()) visitKeys(page.children[i], visit);
            visit(page.keys[i]);
        }
        if (!page.children.empty()) visitKeys(page.children.back(), visit);
    }
};


#include <list>

class PageCache {
    std::size_t capacity;
    std::list<int> recent;
    std::vector<std::list<int>::iterator> location;
    long long reads = 0;
public:
    PageCache(std::size_t count, std::size_t capacity)
        : capacity(capacity), location(count, recent.end()) {}

    void visit(int page) {
        if (location[page] != recent.end()) {
            recent.splice(recent.begin(), recent, location[page]);
            return;
        }
        ++reads;
        if (!capacity) return;
        if (recent.size() == capacity) {
            location[recent.back()] = recent.end();
            recent.pop_back();
        }
        recent.push_front(page);
        location[page] = recent.begin();
    }
    long long diskReads() const { return reads; }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::size_t order, n, q, capacity;
    if (!(std::cin >> order >> n >> q >> capacity)) return 0;
    BTree tree(order);
    for (std::size_t i = 0; i < n; ++i) {
        BTree::Key key;
        std::cin >> key;
        tree.insert(key);
    }
    PageCache cache(tree.pages.size(), capacity);
    for (std::size_t i = 0; i < q; ++i) {
        BTree::Key key;
        std::cin >> key;
        tree.search(key, [&](int page) { cache.visit(page); });
    }
    std::cout << cache.diskReads() << '\n';
}
