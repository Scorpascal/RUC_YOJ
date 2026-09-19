#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

namespace Banking
{
   class BankManager; // 前向声明

   class Account
   {
   private:
      const int accountId;
      double balance;
   ____qcodep____

       public : Account(int id, double bal) : accountId(id), balance(bal)
      {
      }

      ____qcodep____

          int
          getId() const
      {
         return accountId;
      }

      void display() const
      {
         std::cout << "Account ID: " << accountId << std::endl;
         std::cout << "Balance: " << balance << std::endl;
         std::cout << "Transfer log: ";
         ____qcodep____ // 输出转账日志
      }

      void addToLog(const std::string &entry) const
      {
         ____qcodep____ // 添加日志条目
      }
   };

   class BankManager
   {
   public:
      static void auditAccount(const Account &acc)
      {
         std::cout << "[Audit] Account ID: " << acc.accountId << std::endl;
         std::cout << "[Audit] Balance: " << acc.balance << std::endl;
      }
   };

   // 转账函数
   void transfer(Account &from, Account &to, double amount)
   {
      std::cout << "=========transfer=============" << std::endl;

      if (from.balance >= amount)
      {
         from.balance -= amount;
         to.balance += amount;

         std::ostringstream stream;
         stream << std::fixed << std::setprecision(2) << amount;
         std::string result = stream.str();
         std::string fromLog = "[Transfer] " + result + " to " + std::to_string(to.getId());
         std::string toLog = "[Transfer] " + result + " from " + std::to_string(from.getId());

         ____qcodep____

      }
   }
}

int main()
{
   using namespace Banking;

   int id1, id2;
   double bal1, bal2, amount;
   std::cin >> id1 >> bal1 >> id2 >> bal2 >> amount;

   Account acc1(id1, bal1);
   Account acc2(id2, bal2);
   acc1.display();
   acc2.display();

   transfer(acc1, acc2, amount); // 执行转账

   BankManager::auditAccount(acc1);
   BankManager::auditAccount(acc2);

   acc1.display();
   acc2.display();

   return 0;
}