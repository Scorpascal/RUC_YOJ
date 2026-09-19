#include <iostream>
#include <vector>
#include <type_traits>
#include <iomanip>
using namespace std;

// 1. 函数模板部分
template <typename T>
____qcodep____
{
   return a + b;
}

template <typename T1, typename T2>
auto max(T1 a, T2 b) -> decltype(a > b ? a : b)
{
   return (a > b) ? a : b;
}

// 2. 类模板部分
template <typename T, int SIZE>
class Array
{
private:
   ____qcodep____ // 定义
       int count;

public:
   Array() : count(0) {}
   bool push(T value)
   {
      if (count < SIZE)
      {
         data[count++] = value;
         return true;
      }
      return false;
   }
   T pop()
   {
      if (count > 0)
      {
         ____qcodep____
      }
      return T();
   }
   int getSize() const
   {
      return count;
   }
};

// 3. 类模板特化
template <>
class Array<bool, 10>
{
private:
   bool bits[10];
   int count;

public:
   Array() : count(0)
   {

      ____qcodep____ // 初始化
   }
   bool push(bool value)
   {
      if (count < 10)
      {
         bits[count++] = value;
         return true;
      }
      return false;
   }
   bool getBit(int index)
   {
      if (index >= 0 && index < 10)
      {
         return bits[index];
      }
      return false;
   }
};

// 4. 多态计算类（静态多态）
template <typename T>
class Calculator
{
public:
   T add(T a, T b)
   {
      return a + b;
   }
   T subtract(T a, T b)
   {
      return a - b;
   }
   template <typename U>
   ____qcodep____ multiply(T a, U b)
   {
      return a * static_cast<T>(b);
   }
};

// 5. 动态多态基类
class Shape
{
public:
   virtual double area() const = 0;
   virtual ~Shape() {}
};

template <typename T>
class Circle : public Shape
{
private:
   T radius;

public:
   Circle(T r) : radius(r) {}
   double area() const override
   {
      return 3.14159 * radius * radius;
   }
};

// 6. 主函数
int main()
{
   // 1. 函数模板测试
   {
      int a1, b1;
      cin >> a1 >> b1;
      cout << add(a1, b1) << endl;

      double a2, b2;
      cin >> a2 >> b2;
      cout << add(a2, b2) << endl;

      int a3;
      double b3;
      cin >> a3 >> b3;
      cout << max(a3, b3) << endl;
   }

   // 2. 非类型模板参数测试
   {
      int size;
      cin >> size;
      Array<int, 5> intArray;
      for (int i = 0; i < size; i++)
      {
         int val;
         cin >> val;
         intArray.push(val);
      }
      cout << intArray.pop() << endl;
      cout << intArray.getSize() << endl;
   }

   // 3. 类模板特化测试
   {
      int count;
      cin >> count;
      Array<bool, 10> boolArray;
      for (int i = 0; i < count; i++)
      {
         bool val;
         cin >> val;
         boolArray.push(val);
      }
      int index;
      cin >> index;
      cout << boolArray.getBit(index) << endl;
   }

   // 4. 静态多态测试
   {
      double a1, b1;
      cin >> a1 >> b1;
      Calculator<double> calc;
      cout << fixed << calc.add(a1, b1) << endl;

      double a2;
      int b2;
      cin >> a2 >> b2;
      cout << fixed << calc.multiply(a2, b2) << endl;
   }

   // 5. 动态多态测试
   {
      double radius;
      cin >> radius;
      Shape *shape = new Circle<double>(radius);
      cout << shape->area() << endl;
      delete shape;
   }

   return 0;
}