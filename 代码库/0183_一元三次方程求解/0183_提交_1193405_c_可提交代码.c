#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static double A,B,C,D;

static double f(double x){
    return ((A*x + B)*x + C)*x + D; // a*x^3 + b*x^2 + c*x + d
}

static double bisection(double l, double r){
    double fl = f(l), fr = f(r);
    if (fabs(fl) < 1e-14) return l;
    if (fabs(fr) < 1e-14) return r;
    for (int i = 0; i < 100; ++i){
        double m = 0.5*(l + r);
        double fm = f(m);
        if (fabs(fm) < 1e-14) return m;
        if (fl * fm < 0){
            r = m; fr = fm;
        } else {
            l = m; fl = fm;
        }
    }
    return 0.5*(l + r);
}

int cmpdbl(const void *p, const void *q){
    double a = *(const double*)p;
    double b = *(const double*)q;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

int main(void){
    if (scanf("%lf %lf %lf %lf", &A, &B, &C, &D) != 4) return 0;

    double roots[3];
    int cnt = 0;

    double steps[] = {0.5, 0.2, 0.05, 0.01};
    int nsteps = sizeof(steps)/sizeof(steps[0]);

    for (int si = 0; si < nsteps && cnt < 3; ++si){
        double step = steps[si];
        double x0 = -100.0;
        double f0 = f(x0);
        for (double x = x0 + step; x <= 100.0 + 1e-12; x += step){
            double fx = f(x);
            if (fabs(f0) < 1e-14){ // exact hit at x - step
                double root = x - step;
                int dup = 0;
                for (int i = 0; i < cnt; ++i) if (fabs(root - roots[i]) < 1e-7) { dup = 1; break; }
                if (!dup && cnt < 3) roots[cnt++] = root;
            }
            if (f0 * fx < 0){
                double left = x - step;
                double right = x;
                double root = bisection(left, right);
                int dup = 0;
                for (int i = 0; i < cnt; ++i) if (fabs(root - roots[i]) < 1e-7) { dup = 1; break; }
                if (!dup && cnt < 3) roots[cnt++] = root;
            }
            f0 = fx;
            if (cnt >= 3) break;
        }
    }

    // 保险处理：如果通过上述步骤仍未找到 3 个根（极少见），在 [-100,100] 扫描更细网格
    if (cnt < 3){
        double step = 0.001;
        double x0 = -100.0;
        double f0 = f(x0);
        for (double x = x0 + step; x <= 100.0 + 1e-12; x += step){
            double fx = f(x);
            if (fabs(f0) < 1e-14){
                double root = x - step;
                int dup = 0;
                for (int i = 0; i < cnt; ++i) if (fabs(root - roots[i]) < 1e-7) { dup = 1; break; }
                if (!dup && cnt < 3) roots[cnt++] = root;
            }
            if (f0 * fx < 0){
                double root = bisection(x - step, x);
                int dup = 0;
                for (int i = 0; i < cnt; ++i) if (fabs(root - roots[i]) < 1e-7) { dup = 1; break; }
                if (!dup && cnt < 3) roots[cnt++] = root;
            }
            f0 = fx;
            if (cnt >= 3) break;
        }
    }

    // 按从小到大排序并输出（题目已保证存在三个不同实根）
    qsort(roots, 3, sizeof(double), cmpdbl);
    printf("%.2f %.2f %.2f\n", roots[0], roots[1], roots[2]);

    return 0;
}