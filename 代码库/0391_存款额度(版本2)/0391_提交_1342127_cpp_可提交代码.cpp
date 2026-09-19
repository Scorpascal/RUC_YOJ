#include <iostream>
using namespace std;

class TimeSavings;
//声明活期储蓄类
class CurrentSavings
{
private:
    int balance;

public:
    CurrentSavings (int balance){
        this->balance = balance;              
    }
    void print() const { cout << balance << endl; }
    int getBalance() const { return balance; } 
};

//声明定期储蓄类
class TimeSavings
{
private:
    int balance;

public:
    TimeSavings(int balance){
        this->balance = balance;
    }
    void print() const { cout << balance << endl; }
    int getBalance() const { return balance; }
};


void printTotalSavings(const TimeSavings &ts, const CurrentSavings &cs)
{
    cout << cs.getBalance() + ts.getBalance() << endl;
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

