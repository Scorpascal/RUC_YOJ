#include <iostream>
#include <iomanip>
using namespace std;
class Complex {
public:
    double real, imag;
    Complex(double r = 0.0, double i = 0.0) : real(r), imag(i) {}

    // Complex + Complex
    Complex operator+(const Complex& other) const {
        return Complex(real + other.real, imag + other.imag);
    }

    // Complex + double
    Complex operator+(double d) const {
        return Complex(real + d, imag);
    }

    // double + Complex
    friend Complex operator+(double d, const Complex& c) {
        return Complex(d + c.real, c.imag);
    }

    // output: (r+xi) with two decimals
    friend ostream& operator<<(ostream& os, const Complex& c) {
        os << fixed << setprecision(2);
        os << "(" << c.real;
        if (c.imag >= 0) os << "+";
        os << c.imag << "i)";
        os << '\n';
        return os;
    }
}
;
int main()
{
    //测试复数加复数
    double real,imag;
    cin>>real>>imag;
    Complex c1(real,imag);
    cin>>real>>imag;
    Complex c2(real,imag);
    Complex c3=c1+c2;
    cout<<"c1+c2=";
    cout<<c3;
 
    //测试复数加实数
    double d;
    cin>>real>>imag;
    cin>>d;
    c3=Complex(real,imag)+d;
    cout<<"c1+d=";
    cout<<c3;
 
    //测试实数加复数
    cin>>d;
    cin>>real>>imag;
    c1=Complex(real,imag);
    c3=d+c1;
    cout<<"d+c1=";
    cout<<c3;
 
    return 0;
}