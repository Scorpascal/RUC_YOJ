#include <bits/stdc++.h>
using namespace std;

struct Pixel {
    int r = 0, g = 0, b = 0;
    bool good = false; // 非坏点
};

static inline int ceil_div(int sum, int cnt) {
    return (sum + cnt - 1) / cnt;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;

    vector<vector<Pixel>> a(n, vector<Pixel>(m));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            string sR, sG, sB;
            int R, G, B;
            cin >> sR >> R >> sG >> G >> sB >> B;
            a[i][j].r = R; a[i][j].g = G; a[i][j].b = B;
            a[i][j].good = !(R == 0 && G == 0 && B == 0);
        }
    }

    const int dx[4] = {-1, 1, 0, 0};
    const int dy[4] = {0, 0, -1, 1};

    auto has_good_neighbor = [&](int x, int y) -> bool {
        for (int k = 0; k < 4; ++k) {
            int nx = x + dx[k], ny = y + dy[k];
            if (nx < 0 || nx >= n || ny < 0 || ny >= m) continue;
            if (a[nx][ny].good) return true;
        }
        return false;
    };

    // 当前轮待修复集合（坏点且至少有一个相邻非坏点）
    vector<pair<int,int>> cur, nextFrontier;

    // 用时间戳去重（避免每轮清空 O(nm) 的 visited）
    vector<vector<int>> mark(n, vector<int>(m, 0));
    int stamp = 1;

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            if (!a[i][j].good && has_good_neighbor(i, j)) {
                if (mark[i][j] != stamp) {
                    mark[i][j] = stamp;
                    cur.push_back({i, j});
                }
            }
        }
    }

    while (!cur.empty()) {
        // 本轮根据“轮开始时的 good 状态”计算修复结果，统一应用
        struct Repair { int x, y, r, g, b; };
        vector<Repair> repairs;
        repairs.reserve(cur.size());

        for (auto [x, y] : cur) {
            if (a[x][y].good) continue; // 理论上不会发生，防御

            int sr = 0, sg = 0, sb = 0, cnt = 0;
            for (int k = 0; k < 4; ++k) {
                int nx = x + dx[k], ny = y + dy[k];
                if (nx < 0 || nx >= n || ny < 0 || ny >= m) continue;
                if (!a[nx][ny].good) continue;
                sr += a[nx][ny].r;
                sg += a[nx][ny].g;
                sb += a[nx][ny].b;
                cnt++;
            }
            if (cnt == 0) continue; // 本轮无法修复
            repairs.push_back({x, y, ceil_div(sr, cnt), ceil_div(sg, cnt), ceil_div(sb, cnt)});
        }

        if (repairs.empty()) break; // 没有任何点能被修复，结束

        // 应用修复
        for (auto &rp : repairs) {
            a[rp.x][rp.y].r = rp.r;
            a[rp.x][rp.y].g = rp.g;
            a[rp.x][rp.y].b = rp.b;
            a[rp.x][rp.y].good = true;
        }

        // 生成下一轮 frontier：本轮新修复点的相邻坏点
        nextFrontier.clear();
        ++stamp;
        for (auto &rp : repairs) {
            for (int k = 0; k < 4; ++k) {
                int nx = rp.x + dx[k], ny = rp.y + dy[k];
                if (nx < 0 || nx >= n || ny < 0 || ny >= m) continue;
                if (a[nx][ny].good) continue; // 只要坏点
                if (mark[nx][ny] == stamp) continue;
                // 此时 nx,ny 一定至少有一个相邻 good（就是 rp 点），下一轮可尝试修复
                mark[nx][ny] = stamp;
                nextFrontier.push_back({nx, ny});
            }
        }

        cur.swap(nextFrontier);
    }

    // 输出
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            if (j) cout << ' ';
            cout << "R " << a[i][j].r << " G " << a[i][j].g << " B " << a[i][j].b;
        }
        cout << "\n";
    }

    return 0;
}