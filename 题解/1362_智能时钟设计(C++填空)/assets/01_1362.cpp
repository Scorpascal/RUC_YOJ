#include <iostream>
#include <cstring>
#include <iomanip>
using namespace std;

struct Date
{
   int year;
   int month;
   int day;

   void setDate(int y, int m, int d)
   {
      ____qcodep____
   }

   void showDate()
   {
      cout << "Date: " << setfill('0')
           << setw(4) << year << "-"
           << setw(2) << month << "-"
           << setw(2) << day << endl;
   }
};

class Clock
{
public:
   // 默认构造函数
   Clock() : date(new Date())
   {
      ____qcodep____
          setTime(0, 0, 0);
      date->setDate(1970, 1, 1);
   }

   // 含参构造函数（仅时间）
   Clock(int h, int m, int s) : date(new Date())
   {
      ____qcodep____
          setTime(h, m, s);
      date->setDate(1970, 1, 1);
   }

   // 拷贝构造函数（深拷贝）
   Clock(const Clock &other) : ____qcodep____
   {
      hour = other.hour;
      minute = other.minute;
      second = other.second;
      ____qcodep____
   }

   // 析构函数
   ~Clock()
   {
      ____qcodep____
   }

   // 设置时间
   void setTime(int h, int m, int s)
   {
      hour = h;
      minute = m;
      second = s;
   }

   // 设置日期和时间
   void setDateTime(int y, int mo, int d, int h, int mi, int s)
   {
      date->setDate(y, mo, d);
      setTime(h, mi, s);
   }

   // 显示时间
   void showTime()
   {
      cout << "Time: " << setfill('0')
           << setw(2) << hour << ":"
           << setw(2) << minute << ":"
           << setw(2) << second << endl;
   }

   // 显示日期和时间
   void showDateTime()
   {
      showTime();
      date->showDate();
   }

   // 静态成员函数获取时钟计数
   static int getClockCount()
   {
      return clockCount;
   }

private:
   // 使用位域优化存储
   unsigned int hour : 5;   // 0-23 (需要5位)
   unsigned int minute : 6; // 0-59 (需要6位)
   unsigned int second : 6; // 0-59 (需要6位)

   // 组合Date对象（使用指针）
   Date *date;

   // 静态成员，记录时钟总数
   static int clockCount;
};

// 初始化静态成员
int Clock::clockCount = 0;

// 测试函数
void testClockSystem()
{
   cout << "=== Test Case 1 ===" << endl;
   Clock c1(10, 30, 0);
   c1.showDateTime();

   cout << "\n=== Test Case 2 ===" << endl;
   Clock c2 = c1;
   c2.showDateTime();
   cout << "Total clocks: " << Clock::getClockCount() << endl;

   cout << "\n=== Test Case 3 ===" << endl;
   Clock *c3 = new Clock();
   c3->setDateTime(2023, 12, 25, 23, 59, 59);
   c3->showDateTime();
   delete c3;
   cout << "Total clocks after delete: " << Clock::getClockCount() << endl;

   cout << "\n=== Test Case 4 ===" << endl;
   Clock c4;
   c4.setDateTime(2024, 1, 1, 0, 0, 1);
   c4.showDateTime();
   cout << "Total clocks: " << Clock::getClockCount() << endl;
}

int main()
{
   testClockSystem();
   cout << "\nFinal clock count: " << Clock::getClockCount() << endl;
   return 0;
}