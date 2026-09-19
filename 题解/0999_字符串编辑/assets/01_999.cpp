#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>

using namespace std;

struct Editor
{

private:
    struct Block
    {
        array<int, 26> counter;
        string str;

        Block() = default;
        Block(const string &_str) : counter(), str(_str)
        {
            for (auto c : str)
                counter[c - 'a'] += 1;
        }
    };

    struct Node
    {
        shared_ptr<Block> block;
        shared_ptr<Node> next;

        Node() = default;
        Node(const shared_ptr<Block> &_data, const shared_ptr<Node> &_next) : block(_data), next(_next) {}
        Node(const Node &o) : block(o.block), next(o.next) {}
    };

    map<int, shared_ptr<Node>> texts;
    map<int, int> texts_size;
    int version_counter;
    int current_version;

    static constexpr int max_versions = 20000;
    static constexpr int min_block_size = 64;
    static constexpr int block_size = 128;
    static constexpr int max_block_size = 256;

public:
    Editor() : texts(), texts_size(), version_counter(0), current_version(0)
    {
        texts[0] = make_shared<Node>(
            make_shared<Block>(),
            make_shared<Node>(
                make_shared<Block>(),
                nullptr));
        texts_size[0] = 0;
    }

private:
    pair<shared_ptr<Node>, shared_ptr<Node>> _make_text(const string &str)
    {
        shared_ptr<Node> start = make_shared<Node>(make_shared<Block>(), nullptr);
        auto ptr = start;
        int c_pos = 0;
        while (c_pos < str.size())
        {
            string substr = str.substr(c_pos, block_size);
            c_pos += block_size;
            ptr->next = make_shared<Node>(make_shared<Block>(substr), nullptr);
            ptr = ptr->next;
        }
        return {start->next, ptr};
    }

    pair<shared_ptr<Node>, shared_ptr<Node>> _get_text(int new_version, int pos, int len)
    {
        auto ptr = texts[new_version]->next;
        shared_ptr<Node> text_begin = nullptr;
        shared_ptr<Node> text_end = nullptr;

        int c_pos = 0;
        while (c_pos < pos)
        {
            c_pos += ptr->block->str.size();
            ptr = ptr->next;
        }
        text_begin = ptr;
        while (c_pos + ptr->block->str.size() < pos + len)
        {
            c_pos += ptr->block->str.size();
            ptr = ptr->next;
        }
        text_end = ptr;
        auto n_text_begin = make_shared<Node>(*text_begin);
        auto n_ptr = n_text_begin;
        while (n_ptr->next != text_end->next)
        {
            n_ptr->next = make_shared<Node>(*n_ptr->next);
            n_ptr = n_ptr->next;
        }
        n_ptr->next = nullptr;
        return {n_text_begin, n_ptr};
    }

    void _copy(int new_version)
    {
        auto ptr = texts[current_version];
        auto n_ptr = make_shared<Node>(*ptr);
        for (auto a = ptr, b = n_ptr; a->next != nullptr; a = a->next, b = b->next)
        {
            b->next = make_shared<Node>(*a->next);
        }
        texts[new_version] = n_ptr;
        texts_size[new_version] = texts_size[current_version];
    }

    void _split(int new_version, int pos)
    {
        auto ptr = texts[new_version]->next;
        int c_pos = 0;
        while (ptr->block->str.size() > 0 && c_pos + ptr->block->str.size() <= pos)
        {
            c_pos += ptr->block->str.size();
            if (ptr->next == nullptr)
                ptr->next = make_shared<Node>(make_shared<Block>(), nullptr);
            ptr = ptr->next;
        }
        if (c_pos != pos)
        {
            auto block = ptr->block;
            auto next = ptr->next;
            auto n_block1 = make_shared<Block>(block->str.substr(0, pos - c_pos));
            auto n_block2 = make_shared<Block>(block->str.substr(pos - c_pos));
            ptr->block = n_block1;
            ptr->next = make_shared<Node>(n_block2, next);
        }
    }

    void _insert(int new_version, int pos, shared_ptr<Node> text_begin, shared_ptr<Node> text_end)
    {
        auto prev_ptr = texts[new_version];
        auto ptr = prev_ptr->next;
        int c_pos = 0;
        while (c_pos < pos)
        {
            c_pos += ptr->block->str.size();
            prev_ptr = ptr;
            ptr = ptr->next;
        }
        assert(c_pos == pos);
        auto next = ptr->next;
        prev_ptr->next = text_begin;
        text_end->next = ptr;
    }

    void _erase(int new_version, int pos, int len)
    {
        auto prev_ptr = texts[new_version];
        auto ptr = prev_ptr->next;
        int c_pos = 0;
        while (c_pos < pos)
        {
            c_pos += ptr->block->str.size();
            prev_ptr = ptr;
            ptr = ptr->next;
        }
        assert(c_pos == pos);
        while (c_pos < pos + len)
        {
            c_pos += ptr->block->str.size();
            ptr = ptr->next;
        }
        prev_ptr->next = ptr;
    }

    int _query(int new_version, int pos, int len, int ch)
    {
        auto ptr = texts[new_version]->next;
        int res = 0;
        int c_pos = 0;
        while (c_pos < pos)
        {
            c_pos += ptr->block->str.size();
            ptr = ptr->next;
        }
        assert(c_pos == pos);
        while (c_pos < pos + len)
        {
            res += ptr->block->counter[ch];
            c_pos += ptr->block->str.size();
            ptr = ptr->next;
        }
        return res;
    }

    void _maintain(int new_version)
    {
        auto prev_ptr = texts[new_version];
        auto ptr = prev_ptr->next;
        while (ptr->next || ptr->block->str.size() > max_block_size)
        {
            if (ptr->block->str.size() < min_block_size)
            {
                auto n_ptr = make_shared<Node>(
                    make_shared<Block>(ptr->block->str + ptr->next->block->str),
                    ptr->next->next);
                prev_ptr->next = n_ptr;
                ptr = n_ptr;
            }
            else if (ptr->block->str.size() > max_block_size)
            {
                auto block = ptr->block;
                ptr->block = make_shared<Block>(block->str.substr(0, block_size));
                auto n_ptr = make_shared<Node>(
                    make_shared<Block>(block->str.substr(block_size)),
                    ptr->next);
                ptr->next = n_ptr;

                prev_ptr = ptr;
                ptr = ptr->next;
            }
            else
            {
                prev_ptr = ptr;
                ptr = ptr->next;
            }
        }
    }

    void _update_version(int new_version)
    {
        int v = (new_version - max_versions);
        if (texts.count(v))
        {
            texts.erase(v);
            texts_size.erase(v);
        }
        current_version = new_version;
    }

    void _print(int new_version)
    {
        string res = "";
        for (auto ptr = texts[new_version]->next; ptr != nullptr; ptr = ptr->next)
        {
            res += ptr->block->str;
        }
        cout << res << endl;
    }

    // ---------------------------------------------------------

public:
    void insert(int pos, const string &str);
    void erase(int pos, int len);
    void copy(int pos1, int len, int pos2);
    int query(int pos, int len, int ch);
    void jump(int version);
    void print();
};
____qcodep____

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(0);
    auto editor = Editor();
    int n;
    cin >> n;
    for (int i = 1; i <= n; i++)
    {
        string cmd;
        cin >> cmd;
        if (cmd == "i")
        {
            int pos;
            string str;
            cin >> pos >> str;
            editor.insert(pos, str);
        }
        else if (cmd == "e")
        {
            int pos, len;
            cin >> pos >> len;
            editor.erase(pos, len);
        }
        else if (cmd == "c")
        {
            int pos1, len, pos2;
            cin >> pos1 >> len >> pos2;
            editor.copy(pos1, len, pos2);
        }
        else if (cmd == "j")
        {
            int v;
            cin >> v;
            editor.jump(v);
        }
        else if (cmd == "q")
        {
            int pos, len;
            string ch;
            cin >> pos >> len >> ch;
            cout << editor.query(pos, len, ch[0] - 'a') << endl;
        }
    }
    editor.print();
    return 0;
}
