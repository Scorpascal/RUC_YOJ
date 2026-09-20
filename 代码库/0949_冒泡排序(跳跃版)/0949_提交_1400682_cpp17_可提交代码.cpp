#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    int n;
    cin >> n;

    vector<int> a(n);
    for (int i = 0; i < n; ++i) {
        cin >> a[i];
    }

    // right 表示本趟最后一个可能参与比较的右端元素下标
    int right = n - 1;

    do {
        // -1 表示本趟没有发生任何交换
        int lastSwap = -1;

        // 普通的一趟冒泡
        for (int i = 0; i < right; ++i) {
            if (a[i] > a[i + 1]) {
                swap(a[i], a[i + 1]);
                lastSwap = i;
            }
        }

        // 输出本趟结果
        for (int i = 0; i < n; ++i) {
            cout << a[i] << " ";
        }
        cout << '\n';

        // 没有交换：已经有序
        // 最后一次交换发生在起始位置：按照题目要求直接停止
        if (lastSwap <= 0) {
            break;
        }

        // 跳跃：下一趟右边界直接来到最后交换位置
        right = lastSwap;

    } while (right > 0);

    return 0;
}