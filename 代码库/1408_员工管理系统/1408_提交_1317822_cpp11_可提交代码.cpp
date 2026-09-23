#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Person
{
   friend class Employee;
       string name;
   int age;
   static int totalPersons;

public:
   Person(string n, int a) : name(n), age(a) // 初始化
   {
      totalPersons++;
   } // 初始化就看这个剧霍根班德看见帅哥 // 初始化
                             virtual ~Person()
   {
      totalPersons--;
   }
   static int getTotal() { return totalPersons; }
   virtual void display() const
   {
      cout << "Name: " << name << ", Age: " << age;
   }
   void setName(string n) { name = n; }
};

class Employee : public Person
{
protected:
   int employeeId;
   double salary;

public:
   Employee(string n, int a, int id, double s) : Person(n, a), employeeId(id), salary(s){}// {} // 初始化列表
   void display() const override
   {
      Person::display();
              cout
          << ", ID: " << employeeId << ", Salary: $" << salary;
   }
   void raiseSalary(double amount) { salary+=amount; } // 加薪
};

class Manager : public Employee
{
   string department;
   double bonus;
   static int totalManagers;

public:
   Manager(string n, int a, int id, double s, string d, double b)
       : Employee(n, a, id, s), department(d), bonus(b) { totalManagers++; }
   ~Manager() { totalManagers--; }
   static int getTotal() { return totalManagers; }
   void display() const override
   {
      Employee::display(); // 调用基类display // 调用基类display
              cout
          << ", Department: " << department
          << ", Bonus: $" << bonus;
   }
   void changeDepartment(string newDept)
   {
      department = newDept;
   }
};

int Person::totalPersons = 0;
int Manager::totalManagers = 0;

void processTestCase(int type, const vector<string> &data)
{
   if (type == 1)
   { // Employee
      Employee e(data[0], stoi(data[1]), stoi(data[2]), stod(data[3]));
      e.display();
      cout << endl;
   }
   else if (type == 2)
   { // Manager
      Manager m(data[0], stoi(data[1]), stoi(data[2]), stod(data[3]), data[4], stod(data[5]));
      m.display();
      cout << endl;
   }
   else if (type == 3)
   { // Employee with raise
      Employee e(data[0], stoi(data[1]), stoi(data[2]), stod(data[3]));
      e.display();
      cout << endl;
      e.raiseSalary(stod(data[4]));
      e.display();
      cout << endl;
   }
   else if (type == 4)
   { // Manager with change
      Manager m(data[0], stoi(data[1]), stoi(data[2]), stod(data[3]), data[4], stod(data[5]));
      m.display();
      cout << endl;
      m.changeDepartment(data[6]);
      m.raiseSalary(stod(data[7]));
      m.display();
      cout << endl;
   }
}

int main()
{
   int testCaseCount;
   cin >> testCaseCount;
   cin.ignore();

   for (int i = 0; i < testCaseCount; ++i)
   {
      int type;
      cin >> type;
      cin.ignore();

      vector<string> data;
      string line;
      getline(cin, line);

      size_t pos = 0;
      while ((pos = line.find(',')) != string::npos)
      {
         data.push_back(line.substr(0, pos));
         line.erase(0, pos + 1);
      }
      data.push_back(line);

      processTestCase(type, data);
   }

   cout << "Final counts - Persons: " << Person::getTotal()
        << ", Managers: " << Manager::getTotal() << endl;
   return 0;
}