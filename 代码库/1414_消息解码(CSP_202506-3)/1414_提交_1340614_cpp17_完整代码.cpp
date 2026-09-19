#include <bits/stdc++.h>
using namespace std;

using u64 = unsigned long long;
using u32 = unsigned int;
using u128 = unsigned __int128;

static constexpr u64 HASH_C = 47055833459ULL;

static inline u64 parseBits(const string& s, int start, int len) {
    u64 v = 0;
    for (int i = 0; i < len; ++i) v = (v << 1) | (u64)(s[start + i] - '0');
    return v;
}

// 典型代号短数字表示 -> 代号字符串（去掉左侧填充空格）
static inline string decodeTypical(u32 v) {
    const u32 pow26_3 = 26u * 26u * 26u;     // 17576
    const u32 f1 = 36u * 10u * pow26_3;      // 6327360
    const u32 f2 = 10u * pow26_3;            // 175760
    const u32 f3 = pow26_3;                  // 17576
    const u32 f4 = 26u * 26u;                // 676
    const u32 f5 = 26u;

    u32 c1 = v / f1; v %= f1;
    u32 c2 = v / f2; v %= f2;
    u32 c3 = v / f3; v %= f3;
    u32 c4 = v / f4; v %= f4;
    u32 c5 = v / f5; v %= f5;
    u32 c6 = v;

    auto mapC1 = [](u32 x) -> char {
        if (x == 0) return ' ';
        if (1 <= x && x <= 10) return char('0' + (int)x - 1);
        return char('A' + (int)x - 11); // 11..36
    };
    auto mapC2 = [](u32 x) -> char {
        if (0 <= x && x <= 9) return char('0' + (int)x);
        return char('A' + (int)x - 10); // 10..35
    };
    auto mapC3 = [](u32 x) -> char {
        return char('0' + (int)x); // 0..9
    };
    auto mapC4 = [](u32 x) -> char {
        return char('A' + (int)x); // 0..25
    };

    string s;
    s.push_back(mapC1(c1));
    s.push_back(mapC2(c2));
    s.push_back(mapC3(c3));
    s.push_back(mapC4(c4));
    s.push_back(mapC4(c5));
    s.push_back(mapC4(c6));

    // 去掉左侧空格（典型代号是从左侧补空格到 6 位）
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    return s;
}

// 11位 base-38 数字表示 -> 代号字符串（去掉右侧填充空格）
static inline string decodeBase38_11(u64 num) {
    int d[11];
    for (int i = 10; i >= 0; --i) {
        d[i] = (int)(num % 38ULL);
        num /= 38ULL;
    }

    auto digitToChar = [](int x) -> char {
        if (x == 0) return ' ';
        if (1 <= x && x <= 10) return char('0' + x - 1);
        if (11 <= x && x <= 36) return char('A' + x - 11);
        return '_'; // 37
    };

    string s;
    s.reserve(11);
    for (int i = 0; i < 11; ++i) s.push_back(digitToChar(d[i]));

    // 去掉右侧空格（代号从右侧补空格到 11 位）
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

// 代号字符串 -> 11位 base-38 数字表示（右侧补空格）
static inline u64 numericRep11(const string& code) {
    u64 num = 0;
    for (int i = 0; i < 11; ++i) {
        char c = (i < (int)code.size() ? code[i] : ' ');
        int v = 0;
        if (c == ' ') v = 0;
        else if ('0' <= c && c <= '9') v = (c - '0') + 1;     // 1..10
        else if ('A' <= c && c <= 'Z') v = (c - 'A') + 11;    // 11..36
        else if (c == '_') v = 37;
        else v = 0; // 题面外字符，按空格处理（理论不会发生）
        num = num * 38ULL + (u64)v;
    }
    return num;
}

static inline u32 hashN(const string& code, int n) {
    u64 num = numericRep11(code);
    u128 prod = (u128)num * (u128)HASH_C;
    int shift = 64 - n;
    u64 t = (u64)(prod >> shift);
    u64 mask = (n == 64) ? ~0ULL : ((1ULL << n) - 1ULL);
    return (u32)(t & mask);
}

struct HashPair { u32 h25, h12; };

struct Part {
    string code;   // 如果 unknown 则为空
    bool inferred = false; // 需要加 '#'
    bool explicitAppear = false; // 是否“显式出现”（用于更新推断库）
    bool unknown = false; // 输出 ###

    string out() const {
        if (unknown) return "###";
        if (inferred) return "#" + code;
        return code;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    if (!(cin >> n)) return 0;

    unordered_map<u32, string> best25; // hash25 -> 最优(最近/发送优先)显式代号
    unordered_map<u32, string> best12; // hash12 -> 最优(最近/发送优先)显式代号
    unordered_map<string, HashPair> cache; // 代号 -> (h25,h12)

    auto getHashes = [&](const string& code) -> HashPair {
        auto it = cache.find(code);
        if (it != cache.end()) return it->second;
        HashPair hp{hashN(code, 25), hashN(code, 12)};
        cache.emplace(code, hp);
        return hp;
    };

    auto infer25 = [&](u32 h) -> Part {
        auto it = best25.find(h);
        if (it == best25.end()) return Part{"", false, false, true};
        return Part{it->second, true, false, false};
    };

    auto infer12 = [&](u32 h) -> Part {
        auto it = best12.find(h);
        if (it == best12.end()) return Part{"", false, false, true};
        return Part{it->second, true, false, false};
    };

    auto decode28 = [&](u32 v) -> Part {
        const u32 TH = (1u << 25);
        if (v >= TH) {
            string code = decodeTypical(v - TH);
            return Part{code, false, true, false}; // 显式典型代号
        }
        // 25位散列值推断
        return infer25(v);
    };

    for (int i = 0; i < n; ++i) {
        string s;
        cin >> s; // 72 bits

        // 注意：本条消息解码时不得使用本条消息中出现的显式代号，所以先解码输出，后更新 best*
        string outLine;

        vector<pair<bool, string>> toUpdate; 
        // pair<isSender, code>，用于按“先收后发”更新，使同消息内哈希相同则发送方覆盖

        if (s[0] == '0') {
            u32 recvV = (u32)parseBits(s, 1, 28);
            u32 sendV = (u32)parseBits(s, 29, 28);
            u32 posV  = (u32)parseBits(s, 57, 15);

            Part recv = decode28(recvV);
            Part send = decode28(sendV);

            outLine = recv.out() + " " + send.out();
            if (posV != 0) outLine += " " + to_string(posV);

            // 收集本条消息显式出现的代号用于更新（推断得到的不更新）
            if (recv.explicitAppear && !recv.unknown) toUpdate.push_back({false, recv.code});
            if (send.explicitAppear && !send.unknown) toUpdate.push_back({true,  send.code});
        } else {
            u64 num58 = parseBits(s, 1, 58);
            u32 h12v  = (u32)parseBits(s, 59, 12);
            int flag  = (int)(s[71] - '0');

            string firstCode = decodeBase38_11(num58);
            Part other = infer12(h12v);

            Part receiver, sender;

            if (flag == 0) { // 首先的一方是发送方
                sender = Part{firstCode, false, true, false}; // 显式
                receiver = other; // 推断/未知
            } else { // 首先的一方是接收方
                receiver = Part{firstCode, false, true, false}; // 显式
                sender = other; // 推断/未知
            }

            outLine = receiver.out() + " " + sender.out();

            // 仅把显式出现的 firstCode 更新到对应角色
            if (flag == 0) toUpdate.push_back({true, firstCode});   // sender
            else           toUpdate.push_back({false, firstCode});  // receiver
        }

        cout << outLine << "\n";

        // 更新推断库：按“先收后发”顺序更新，保证同一条消息内若二者哈希都匹配则发送方优先
        // toUpdate 里已经按收/发顺序压入（简单消息是收后发；复杂消息只有一方）
        for (auto& [isSender, code] : toUpdate) {
            auto hp = getHashes(code);
            // 显式出现的代号才进入推断库
            best25[hp.h25] = code;
            best12[hp.h12] = code;
            (void)isSender; // 角色只影响同消息覆盖顺序，我们已用插入顺序处理
        }
    }

    return 0;
}