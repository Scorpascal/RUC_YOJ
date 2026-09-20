#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
using namespace std;

struct Process {
    char type;
    int st = 0;
    int w = 0;
    int k = 0;

    vector<int> a;
    vector<int> t;

    enum State {
        NOT_STARTED,
        WAIT,
        RUN,
        DONE
    } state = NOT_STARTED;

    int cur = 0;       // 当前任务
    int rem = 0;       // 当前任务剩余运行段数
    int fail = 0;      // 当前资源已经失败次数

    // A 对“当前任务”是否已经触发过释放
    bool a_fired = false;

    int held = 0;      // 当前实际持有资源数

    long long gain = 0;
    long long endSeg = -1;
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;

    vector<Process> p(n);

    int maxStart = 0;

    for (int i = 0; i < n; ++i) {
        cin >> p[i].type >> p[i].st;

        maxStart = max(maxStart, p[i].st);

        if (p[i].type == 'X') {
            cin >> p[i].k;
        } else {
            cin >> p[i].w >> p[i].k;
        }

        p[i].a.resize(p[i].k);
        p[i].t.resize(p[i].k);

        for (int j = 0; j < p[i].k; ++j) {
            cin >> p[i].a[j] >> p[i].t[j];
            --p[i].a[j];
        }
    }

    // owner[x] = 当前持有资源 x 的进程，-1 表示空闲
    vector<int> owner(m, -1);

    auto releaseAll = [&](int id) {
        for (int x = 0; x < m; ++x) {
            if (owner[x] == id) {
                owner[x] = -1;
            }
        }
        p[id].held = 0;
    };

    long long seg = 1;

    while (true) {
        bool changed = false;

        /*
         * =========================
         * 1. 段首：新进程开始活动
         * =========================
         */
        for (int i = 0; i < n; ++i) {
            if (p[i].state == Process::NOT_STARTED &&
                p[i].st == seg) {

                p[i].state = Process::WAIT;
                p[i].cur = 0;
                p[i].fail = 0;
                p[i].a_fired = false;

                changed = true;
            }
        }

        /*
         * requests[x]:
         * 当前段首正在申请资源 x 的所有进程。
         */
        vector<vector<int>> requests(m);

        for (int i = 0; i < n; ++i) {
            if (p[i].state == Process::WAIT) {
                int x = p[i].a[p[i].cur];
                requests[x].push_back(i);
            }
        }

        vector<int> failed;
        vector<int> releaseA;

        /*
         * 让一个进程从本段开始进入任务运行。
         */
        auto startRun = [&](int id) {
            p[id].state = Process::RUN;
            p[id].rem = p[id].t[p[id].cur];
            changed = true;
        };

        /*
         * =========================
         * 2. 段首：处理所有资源申请
         * =========================
         */
        for (int x = 0; x < m; ++x) {
            if (requests[x].empty()) {
                continue;
            }

            /*
             * 找出本段已经到达抢夺时刻的 C。
             *
             * C 在前 w 次申请全部失败后，
             * 下一段首抢夺。
             *
             * 因此：
             * fail == w 代表本段应该抢夺。
             */
            vector<int> stealers;

            for (int id : requests[x]) {
                if (p[id].type == 'C' &&
                    p[id].fail == p[id].w) {
                    stealers.push_back(id);
                }
            }

            /*
             * 情况 1：存在 C 抢夺
             */
            if (!stealers.empty()) {

                // 编号最大的 C 抢夺成功
                int winner = *max_element(
                    stealers.begin(),
                    stealers.end()
                );

                /*
                 * 无视原归属进行抢夺。
                 */
                int old = owner[x];

                if (old != -1) {
                    --p[old].held;
                }

                owner[x] = winner;
                ++p[winner].held;

                changed = true;

                /*
                 * 所有参与抢夺的 C 都进入运行。
                 *
                 * 只有 winner 得到资源，
                 * 其余虽然抢夺失败，但仍开始任务。
                 */
                for (int id : stealers) {
                    startRun(id);
                }

                /*
                 * 其他所有申请者都失败。
                 */
                for (int id : requests[x]) {
                    bool isStealer =
                        (p[id].type == 'C' &&
                         p[id].fail == p[id].w);

                    if (!isStealer) {
                        failed.push_back(id);
                    }
                }
            }

            /*
             * 情况 2：没有 C 抢夺
             */
            else {
                /*
                 * 资源空闲：
                 * 编号最小者成功。
                 */
                if (owner[x] == -1) {

                    int winner = *min_element(
                        requests[x].begin(),
                        requests[x].end()
                    );

                    owner[x] = winner;
                    ++p[winner].held;

                    startRun(winner);

                    for (int id : requests[x]) {
                        if (id != winner) {
                            failed.push_back(id);
                        }
                    }
                }

                /*
                 * 资源已经被占有：
                 * 所有申请失败。
                 */
                else {
                    for (int id : requests[x]) {
                        failed.push_back(id);
                    }
                }
            }
        }

        /*
         * =========================
         * 3. 处理本段申请失败
         * =========================
         */
        for (int id : failed) {
            Process &q = p[id];

            if (q.type == 'X') {
                /*
                 * 普通进程什么都不做。
                 * 下一段继续申请。
                 */
            }

            else if (q.type == 'A') {
                /*
                 * A 只对当前任务触发一次耐心机制。
                 *
                 * 触发以后即使继续无限失败，
                 * 也不会再次释放。
                 */
                if (!q.a_fired) {
                    ++q.fail;
                    changed = true;

                    if (q.fail == q.w) {
                        /*
                         * 注意：
                         * 此处不能立刻 releaseAll。
                         *
                         * A 是“本段末”释放。
                         */
                        q.a_fired = true;
                        releaseA.push_back(id);
                    }
                }
            }

            else if (q.type == 'B') {
                ++q.fail;
                changed = true;

                /*
                 * 第 w 次失败的这个段首，
                 * 立即放弃当前资源并进入任务。
                 */
                if (q.fail == q.w) {
                    startRun(id);
                }
            }

            else if (q.type == 'C') {
                /*
                 * C 前 w 次只是正常失败。
                 *
                 * 当 fail 变成 w 后，
                 * 下一段才进行抢夺。
                 */
                ++q.fail;
                changed = true;
            }
        }

        /*
         * =========================
         * 4. 段中：计算收益
         * =========================
         *
         * 只有 RUN 状态有收益。
         *
         * 注意 C 在段首已经完成资源抢夺，
         * 所以这里使用的是抢夺后的 held。
         */
        for (int i = 0; i < n; ++i) {
            if (p[i].state == Process::RUN) {
                p[i].gain += p[i].held;
            }
        }

        /*
         * =========================
         * 5. 段末：运行时间减少
         * =========================
         */
        for (int i = 0; i < n; ++i) {
            if (p[i].state != Process::RUN) {
                continue;
            }

            --p[i].rem;
            changed = true;

            /*
             * 当前任务结束。
             */
            if (p[i].rem == 0) {

                ++p[i].cur;

                /*
                 * 全部任务完成。
                 */
                if (p[i].cur == p[i].k) {
                    p[i].state = Process::DONE;
                    p[i].endSeg = seg;

                    /*
                     * 全部任务完成的当段末
                     * 释放所有资源。
                     */
                    releaseAll(i);
                }

                /*
                 * 下一任务要到下一段首才申请。
                 */
                else {
                    p[i].state = Process::WAIT;

                    // 新任务重新计算失败次数
                    p[i].fail = 0;
                    p[i].a_fired = false;
                }
            }
        }

        /*
         * =========================
         * 6. 段末：执行 A 的释放
         * =========================
         */
        for (int id : releaseA) {
            releaseAll(id);
        }

        /*
         * 全部正常结束。
         */
        bool allDone = true;

        for (int i = 0; i < n; ++i) {
            if (p[i].state != Process::DONE) {
                allDone = false;
                break;
            }
        }

        if (allDone) {
            break;
        }

        /*
         * =========================
         * 7. 永久死锁判断
         * =========================
         *
         * 所有进程都已经过了启动时刻，
         * 且这一整段没有任何状态变化。
         *
         * 下一段一定与这一段完全一样，
         * 所以以后永远无法继续。
         */
        if (seg >= maxStart && !changed) {
            break;
        }

        ++seg;
    }

    /*
     * =========================
     * 输出
     * =========================
     */
    for (int i = 0; i < n; ++i) {
        cout << p[i].gain << ' ';

        if (p[i].state == Process::DONE) {
            /*
             * 从 st 段到 endSeg 段，
             * 两端都算，因此 +1。
             */
            cout << p[i].endSeg - p[i].st + 1;
        } else {
            cout << -1;
        }

        cout << '\n';
    }

    return 0;
}