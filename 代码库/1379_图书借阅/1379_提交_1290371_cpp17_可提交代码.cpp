#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

namespace LibrarySystem
{

   class Patron; // 前向声明

   class Book
   {
   private:
      const int bookId;
      std::string title;
      bool isAvailable;
   int borrowedBy;
   mutable int borrowCount;

       public : Book(int id, const std::string &t)
       : bookId(id), title(t), isAvailable(true), borrowCount(0)
      {
      }

      friend class Librarian;

          int
          getId() const
      {
         return bookId;
      }
      std::string getTitle() const { return title; }
      bool getStatus() const { return isAvailable; }

      void display() const
      {
         std::cout << "Book ID: " << bookId << std::endl;
         std::cout << "Title: " << title << std::endl;
         std::cout << "Status: " << (isAvailable ? "Available" : "Borrowed by " + std::to_string(borrowedBy)) << std::endl;
      }

      void incrementBorrowCount() const
      {
         borrowCount++;
      }
   };

   class Patron
   {
   private:
      const int patronId;
      std::string name;
      std::vector<int> borrowedBooks; // 借阅的图书ID列表

   public:
      Patron(int id, const std::string &n) : patronId(id), name(n) {}

      friend class Librarian; // 声明友元函数

      int getId() const { return patronId; }
      std::string getName() const { return name; }

      void display() const
      {
         std::cout << "Patron ID: " << patronId << std::endl;
         std::cout << "Name: " << name << std::endl;
         std::cout << "Borrowed books: ";
         for (int id : borrowedBooks)
         {
            std::cout << id << " ";
         }
         std::cout << "\nTotal borrowed: " << borrowedBooks.size() << std::endl;
      }

      void borrowBook(int bookId)
      {
         borrowedBooks.push_back(bookId);
      }

      void returnBook(int bookId)
      {
         borrowedBooks.erase(std::remove(borrowedBooks.begin(), borrowedBooks.end(), bookId), borrowedBooks.end());
      }
   };

   class Librarian
   {
   public:
      static void processBorrow(Book &book, Patron &patron)
      {
         if (book.getStatus())
         {
            book.isAvailable = false;
            book.borrowedBy = patron.getId();
            patron.borrowBook(book.getId());
            book.incrementBorrowCount();

                    std::cout
                << "===== Borrow Operation =====" << std::endl;
         }
      }
   };

}

int main()
{
   using namespace LibrarySystem;

   int bookId1, bookId2, patronId;
   std::string title1, title2, name;
   int dummy;

   // 读取第一本书的信息
   std::cin >> bookId1;            // 读取 bookId1
   std::cin.ignore();              // 忽略紧接的换行符或空格
   std::getline(std::cin, title1); // 读取标题1及操作码
   size_t pos = title1.find_last_of(' ');
   if (pos != std::string::npos)
   { // 分离操作码
      dummy = std::stoi(title1.substr(pos + 1));
      title1 = title1.substr(0, pos);
   }

   // 读取第二本书的信息
   std::cin >> bookId2;
   std::cin.ignore();
   std::getline(std::cin, title2);
   pos = title2.find_last_of(' ');
   if (pos != std::string::npos)
   {
      dummy = std::stoi(title2.substr(pos + 1));
      title2 = title2.substr(0, pos);
   }

   // 读取读者信息
   std::cin >> patronId;
   std::cin.ignore();
   std::getline(std::cin, name);
   pos = name.find_last_of(' ');
   if (pos != std::string::npos)
   {
      dummy = std::stoi(name.substr(pos + 1));
      name = name.substr(0, pos);
   }

   Book book1(bookId1, title1);
   Book book2(bookId2, title2);
   Patron patron(patronId, name);

   book1.display();
   book2.display();
   patron.display();

   Librarian::processBorrow(book1, patron);

       book1.display();
   book2.display();
   patron.display();

   return 0;
}