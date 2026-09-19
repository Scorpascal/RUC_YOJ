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
   Polynomial operator+(const Polynomial&a)const{
        vector<double> t=coefficients;
        for(int i=0;i<a.coefficients.size();i++){
            if(i>=coefficients.size()){
                t.push_back(a.coefficients[i]);
            }
            else t[i]+=a.coefficients[i];
        }
        return Polynomial(t);
   }

       // 友元函数：重载减法运算符
       friend Polynomial operator-(const Polynomial&a,const Polynomial&b);

           // 求值函数
           double evaluate(double a){
            double sum=0;
            for(int i=0;i<coefficients.size();i++){
                sum+=pow(a,i)*coefficients[i];
            }
            return sum;
           }

       // 声明友元输出运算符
       friend ostream&operator<<(ostream&a,const Polynomial&b);
};

// 实现友元输出运算符
ostream&operator<<(ostream&a,const Polynomial&b){
    int flag=0;
    for(int i=b.coefficients.size()-1;i>=0;i--){
        
            if(b.coefficients[i]==0){
                a<<"+0";continue;
            }
            else if(b.coefficients[i]>0){
                if(flag){
                    a<<"+";
                    
                }
                flag++;
                if(b.coefficients[i]!=1||i==0){
                    a<<b.coefficients[i];
                }
                
            }
            else{
                flag++;
                if(b.coefficients[i]!=-1||i==0){
                    a<<b.coefficients[i];
                }
                else a<<"-";
            }
            if(i>0)a<<"x";
            if(i>1)a<<"^"<<i;
        }
    
    return a;
}

    // 实现友元减法运算符
    Polynomial operator-(const Polynomial&a,const Polynomial&b){
        vector<double> t=a.coefficients;
        for(int i=0;i<b.coefficients.size();i++){
            if(i>=a.coefficients.size()){
                t.push_back(-b.coefficients[i]);
            }
            else t[i]-=b.coefficients[i];
        }
        return Polynomial(t);
    }

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
