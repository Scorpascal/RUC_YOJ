#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdio>
using namespace std;

const int MAXN = 100010;

struct Event {
    int x;
    int y1, y2;
    int type; // 1: start, -1: end
    Event() {}
    Event(int x, int y1, int y2, int type) : x(x), y1(y1), y2(y2), type(type) {}
    bool operator < (const Event &e) const {
        return x < e.x;
    }
};

int n;
vector<Event> events;
vector<int> ys;

int cnt[MAXN * 8];
long long len[MAXN * 8];

void build(int idx, int l, int r) {
    cnt[idx] = 0;
    len[idx] = 0;
    if (l == r) return;
    int mid = (l + r) >> 1;
    build(idx << 1, l, mid);
    build(idx << 1 | 1, mid + 1, r);
}

void push_up(int idx, int l, int r) {
    if (cnt[idx] > 0) {
        len[idx] = ys[r + 1] - ys[l];
    } else {
        if (l == r) {
            len[idx] = 0;
        } else {
            len[idx] = len[idx << 1] + len[idx << 1 | 1];
        }
    }
}

void update(int idx, int l, int r, int L, int R, int val) {
    if (L <= l && r <= R) {
        cnt[idx] += val;
        push_up(idx, l, r);
        return;
    }
    int mid = (l + r) >> 1;
    if (L <= mid) update(idx << 1, l, mid, L, R, val);
    if (R > mid) update(idx << 1 | 1, mid + 1, r, L, R, val);
    push_up(idx, l, r);
}

int main() {
    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        int x1, y1, x2, y2;
        scanf("%d%d%d%d", &x1, &y1, &x2, &y2);
        events.push_back(Event(x1, y2, y1, 1));
        events.push_back(Event(x2, y2, y1, -1));
        ys.push_back(y1);
        ys.push_back(y2);
    }
    
    if (events.empty()) {
        printf("0\n");
        return 0;
    }
    
    sort(ys.begin(), ys.end());
    ys.erase(unique(ys.begin(), ys.end()), ys.end());
    
    int m = ys.size() - 1;
    if (m <= 0) {
        printf("0\n");
        return 0;
    }
    
    build(1, 0, m - 1);
    
    sort(events.begin(), events.end());
    
    long long ans = 0;
    int last_x = events[0].x;
    for (int i = 0; i < events.size(); i++) {
        Event e = events[i];
        if (e.x != last_x) {
            ans += (long long)(e.x - last_x) * len[1];
            last_x = e.x;
        }
        int l = lower_bound(ys.begin(), ys.end(), e.y1) - ys.begin();
        int r = lower_bound(ys.begin(), ys.end(), e.y2) - ys.begin();
        if (l < r) {
            update(1, 0, m - 1, l, r - 1, e.type);
        }
    }
    
    printf("%lld\n", ans);
    return 0;
}