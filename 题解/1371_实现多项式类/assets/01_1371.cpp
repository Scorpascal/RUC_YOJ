#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
using namespace std;

class Polynomial
{
private:
   vector<double> coefficients; // 系数，从低次项到高次项存储
public:
   Polynomial(const vector<double> &coeffs) : coefficients(coeffs) {}

   // 成员函数：重载加法运算符
   ____qcodep____

       // 友元函数：重载减法运算符
       ____qcodep____

           // 求值函数
           ____qcodep____

       // 声明友元输出运算符
       ____qcodep____
};

// 实现友元输出运算符
____qcodep____

    // 实现友元减法运算符
    ____qcodep____

    // 函数指针类型
    typedef Polynomial (*PolyOperation)(const Polynomial &, const Polynomial &);

int main()
{
   int n, m;
   cin >> n;
   vector<double> coeffs1(n + 1);
   for (double &c : coeffs1)
      cin >> c;

   cin >> m;
   vector<double> coeffs2(m + 1);
   for (double &c : coeffs2)
      cin >> c;

   Polynomial p1(coeffs1), p2(coeffs2);

   // 测试加法
   Polynomial sum = p1 + p2;
   cout << "Sum: " << sum << endl;

   // 测试减法
   Polynomial diff = p1 - p2;
   cout << "Difference: " << diff << endl;

   // 测试求值
   double x;
   cin >> x;
   cout << "P1(" << x << ") = " << p1.evaluate(x) << endl;
   cout << "P2(" << x << ") = " << p2.evaluate(x) << endl;

   // 测试函数指针
   PolyOperation op = [](const Polynomial &a, const Polynomial &b)
   {
      return a + b;
   };
   cout << "Function pointer (addition): " << op(p1, p2) << endl;

   return 0;
}
