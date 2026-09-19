#include <iostream>
#include <iomanip>
using namespace std;
____qcodep____
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