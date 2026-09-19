#include <iostream>
using namespace std;

____qcodep____
//声明活期储蓄类
class CurrentSavings
{
private:
    int balance;

public:
    CurrentSavings (int balance){
        ____qcodep____        
    }
    ____qcodep____ 
};

//声明定期储蓄类
class TimeSavings
{
private:
    int balance;

public:
    TimeSavings(int balance){
        ____qcodep____
    }
    ____qcodep____
};


void printTotalSavings(const TimeSavings &ts, const CurrentSavings &cs)
{
    ____qcodep____
}

int main(void)
{
    int cs, ts;
    cin >> cs >> ts;

    CurrentSavings cs1(cs);
    TimeSavings ts1(ts);

    cs1.print();
    ts1.print();
    printTotalSavings(ts1, cs1);

    return 0;
}

