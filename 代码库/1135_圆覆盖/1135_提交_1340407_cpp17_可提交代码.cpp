#include <bits/stdc++.h>
using namespace std;

struct Point {
    double x, y;
};

struct Circle {
    Point c;
    double r; // radius
};

static inline double sqr(double v) { return v * v; }

static inline double dist2(const Point& a, const Point& b) {
    return sqr(a.x - b.x) + sqr(a.y - b.y);
}

static inline bool inCircle(const Point& p, const Circle& cir) {
    // 允许一点点数值误差
    const double eps = 1e-12;
    return dist2(p, cir.c) <= sqr(cir.r) + eps;
}

static inline Circle circleFrom1(const Point& a) {
    return Circle{a, 0.0};
}

static inline Circle circleFrom2(const Point& a, const Point& b) {
    Point c{ (a.x + b.x) / 2.0, (a.y + b.y) / 2.0 };
    double r = sqrt(dist2(a, c));
    return Circle{c, r};
}

static inline Circle circleFrom3(const Point& a, const Point& b, const Point& c) {
    // 外接圆：使用行列式公式求圆心
    long double ax = a.x, ay = a.y;
    long double bx = b.x, by = b.y;
    long double cx = c.x, cy = c.y;

    long double d = 2.0L * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    const long double eps = 1e-18L;

    if (fabsl(d) < eps) {
        // 三点近共线：最小圆必为覆盖三点的某个两点直径圆（即由最远点对决定）
        Circle c1 = circleFrom2(a, b);
        Circle c2 = circleFrom2(a, c);
        Circle c3 = circleFrom2(b, c);

        Circle best = c1;
        if (c2.r < best.r) best = c2;
        if (c3.r < best.r) best = c3;

        // 确保覆盖三点：若最小半径那个不覆盖，改用能覆盖的最小者
        vector<Circle> cand{c1, c2, c3};
        Circle ans = cand[0];
        ans.r = 1e300; // INF
        for (auto &cc : cand) {
            if (inCircle(a, cc) && inCircle(b, cc) && inCircle(c, cc)) {
                if (cc.r < ans.r) ans = cc;
            }
        }
        if (ans.r < 1e299) return ans;

        // 理论上不会走到这里
        return best;
    }

    long double a2 = ax * ax + ay * ay;
    long double b2 = bx * bx + by * by;
    long double c2 = cx * cx + cy * cy;

    long double ux = (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / d;
    long double uy = (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / d;

    Point center{ (double)ux, (double)uy };
    double r = sqrt(dist2(center, a));
    return Circle{center, r};
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;
    vector<Point> p;
    p.reserve((size_t)n);

    for (int i = 0; i < n; ++i) {
        Point pt;
        cin >> pt.x >> pt.y;
        p.push_back(pt);
    }

    // 随机打乱，保证期望线性
    std::mt19937_64 rng(
        (uint64_t)chrono::high_resolution_clock::now().time_since_epoch().count()
    );
    shuffle(p.begin(), p.end(), rng);

    Circle cir{{0.0, 0.0}, -1.0};

    for (int i = 0; i < n; ++i) {
        if (cir.r >= 0.0 && inCircle(p[i], cir)) continue;

        cir = circleFrom1(p[i]);
        for (int j = 0; j < i; ++j) {
            if (inCircle(p[j], cir)) continue;

            cir = circleFrom2(p[i], p[j]);
            for (int k = 0; k < j; ++k) {
                if (inCircle(p[k], cir)) continue;

                cir = circleFrom3(p[i], p[j], p[k]);
            }
        }
    }

    cout.setf(std::ios::fixed);
    cout << setprecision(2) << cir.c.x << ' ' << cir.c.y << ' ' << cir.r << "\n";
    return 0;
}