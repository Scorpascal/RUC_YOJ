#include <algorithm>
#include <iostream>
#include <queue>
#include <vector>
using namespace std;

struct PairValue {
    long long cost;
    long long negativeCount;
};

struct PairGreater {
    bool operator()(const PairValue &x, const PairValue &y) const {
        if (x.cost != y.cost) return x.cost > y.cost;
        return x.negativeCount > y.negativeCount;
    }
};

struct Evaluation {
    long long count;
    long long realCost;
};

int n;
long long budget;
vector<long long> makeCost, checkCost;

PairValue addPair(const PairValue &x, const PairValue &y) {
    return {x.cost + y.cost, x.negativeCount + y.negativeCount};
}

PairValue subtractPair(const PairValue &x, const PairValue &y) {
    return {x.cost - y.cost, x.negativeCount - y.negativeCount};
}

bool pairLess(const PairValue &x, const PairValue &y) {
    if (x.cost != y.cost) return x.cost < y.cost;
    return x.negativeCount < y.negativeCount;
}

Evaluation evaluate(long long rewardTwice) {
    priority_queue<PairValue, vector<PairValue>, PairGreater> slopes;
    PairValue functionAtZero{0, 0};

    for (int day = 1; day <= n; ++day) {
        PairValue idle{0, 0};
        PairValue both{2LL * (makeCost[day] + checkCost[day]) - rewardTwice, -1};
        PairValue center = pairLess(both, idle) ? both : idle;
        PairValue down{2LL * checkCost[day] - rewardTwice, -1};
        PairValue up{2LL * makeCost[day], 0};
        PairValue lower = subtractPair(center, down);
        PairValue upper = subtractPair(up, center);

        functionAtZero = addPair(functionAtZero, center);
        if (!slopes.empty() && pairLess(slopes.top(), lower)) {
            PairValue smallest = slopes.top();
            slopes.pop();
            functionAtZero = addPair(functionAtZero,
                                     subtractPair(smallest, lower));
            slopes.push(lower);
        }
        slopes.push(upper);
    }

    long long count = -functionAtZero.negativeCount;
    long long realCostTwice = functionAtZero.cost + rewardTwice * count;
    return {count, realCostTwice / 2};
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (!(cin >> n >> budget)) return 0;
    makeCost.assign(n + 1, 0);
    checkCost.assign(n + 1, 0);
    for (int i = 1; i <= n; ++i) cin >> makeCost[i];
    for (int i = 1; i <= n; ++i) cin >> checkCost[i];

    long long low = 0, high = 1;
    while (evaluate(high).count < n) high <<= 1;
    while (low < high) {
        long long middle = low + (high - low + 1) / 2;
        Evaluation current = evaluate(middle);
        if (current.realCost <= budget) low = middle;
        else high = middle - 1;
    }

    Evaluation left = evaluate(low);
    if (left.count == n) {
        cout << n << '\n';
        return 0;
    }
    Evaluation right = evaluate(low + 1);
    if (right.realCost <= budget) {
        cout << right.count << '\n';
        return 0;
    }
    if (right.count == left.count) {
        cout << left.count << '\n';
        return 0;
    }
    long long marginal = (right.realCost - left.realCost) /
                         (right.count - left.count);
    long long extra = (budget - left.realCost) / marginal;
    extra = min(extra, right.count - left.count);
    cout << left.count + extra << '\n';
    return 0;
}