#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
using namespace std;

// 异常类：用于2D图形调用体积计算时抛出
class Not3DException : public runtime_error
{
public:
   Not3DException() : runtime_error("Cannot compute volume for 2D shape!") {}
};

// 图形基类
class Shape
{
protected:
   string name;
   bool is3D;

public:
   Shape(string n, bool d) : name(n), is3D(d) {}
   virtual double getArea() const = 0; // 纯虚函数：获取面积/表面积
   virtual double getVolume() const
   {                          // 虚函数：获取体积（3D图形特有）
      throw Not3DException(); // 基类默认实现抛出异常
   }
   string getName() const { return name; }
   bool is3DObject() const { return is3D; }
   virtual ~Shape() { cout << "Destroying Shape: " << name << endl; }
};

// 球体类（3D图形）
class Sphere : public Shape
{
private:
   double radius;

public:
   Sphere(string n, double r) : ____qcodep____ // 初始化
                                ____qcodep____ // π使用3.14159
                                ~Sphere()
   {
      cout << "Destroying Sphere: " << name << endl;
   }
};

// 矩形类（2D图形）
class Rectangle : public Shape
{
private:
   double width, height;

public:
   Rectangle(string n, double w, double h) : ____qcodep____ // 初始化基类和成员变量
                                             double getArea() const override
   {
      return width * height; // 矩形面积公式
   }
   ~Rectangle() { cout << "Destroying Rectangle: " << name << endl; }
};

// 立方体类（3D图形）
class Cube : public Shape
{
private:
   double width, height, depth;

public:
   Cube(string n, double w, double h, double d)
       : Shape(n, true), width(w), height(h), depth(d) {}
   double getArea() const override
   {
      return ____qcodep____ // 立方体表面积公式
   }
   double getVolume() const override
   {
      return width * height * depth; // 立方体体积公式
   }
   ~Cube() { cout << "Destroying Cube: " << name << endl; }
};

// 复合图形类（支持2D和3D混合）
class CompositeShape : public Shape
{
private:
   vector<Shape *> children;
   bool ownsChildren; // 是否拥有子图形的所有权

public:
   CompositeShape(string n, bool own = true)
       : Shape(n, false), ownsChildren(own) {}
   void addShape(Shape *s)
   {
      children.push_back(s);
      if (ownsChildren && s->is3DObject())
      {
         is3D = true; // 若包含3D图形，则自身标记为3D
      }
   }
   double getArea() const override
   {
      double total = 0;
      for (Shape *s : children)
      {
         total += s->getArea(); // 累加子图形面积
      }
      return total;
   }
   double getVolume() const override
   {
      if (!is3D)
      {
         throw Not3DException(); // 2D复合图形调用体积计算抛出异常
      }
      double total = 0;
      for (Shape *s : children)
      {
         if (s->is3DObject())
         {
            total += s->getVolume(); // 仅累加3D子图形的体积
         }
      }
      return total;
   }
   ____qcodep____ // 析构函数
};

// 输入处理与测试函数
void runTest()
{
   int n;
   cin >> n;
   cin.ignore(); // 忽略换行符

   CompositeShape scene("Scene", true); // 创建场景，默认拥有子图形所有权

   for (int i = 0; i < n; i++)
   {
      string line, type, name;
      getline(cin, line);
      size_t pos = line.find(',');
      type = line.substr(0, pos);
      line.erase(0, pos + 1);

      pos = line.find(',');
      name = line.substr(0, pos);
      line.erase(0, pos + 1);

      if (type == "Sphere")
      {
         double r = stod(line);
         scene.addShape(new Sphere(name, r));
      }
      else if (type == "Rectangle")
      {
         pos = line.find(',');
         double w = stod(line.substr(0, pos));
         double h = stod(line.substr(pos + 1));
         scene.addShape(new Rectangle(name, w, h));
      }
      else if (type == "Cube")
      {
         pos = line.find(',');
         double w = stod(line.substr(0, pos));
         line.erase(0, pos + 1);
         pos = line.find(',');
         double h = stod(line.substr(0, pos));
         double d = stod(line.substr(pos + 1));
         scene.addShape(new Cube(name, w, h, d));
      }
      else if (type == "Composite")
      {
         // 简化处理复合图形的创建，实际可递归处理
         bool own = (line == "True") ? true : false;
         CompositeShape *comp = new CompositeShape(name, own);
         scene.addShape(comp);
      }
   }

   cout << "Scene contains " << (scene.is3DObject() ? "3D" : "2D") << " objects" << endl;
   cout << "Total area: " << scene.getArea() << endl;
   try
   {
      cout << "Total volume: " << scene.getVolume() << endl;
   }
   catch (const exception &e)
   {
      cout << "Error: " << e.what() << endl;
   }
}

int main()
{
   runTest();
   return 0;
}