#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <string>
#include <memory>
#include <stdexcept>
#include <limits>
#include <type_traits>

// 1. 数据存储类模板
template <typename T, size_t DIM>
class DataArray
{
private:
   T data[DIM];
   size_t size = 0;

public:
   bool push(const T &val)
   {
      if (size < DIM)
      {
         data[size++] = val;
         return true;
      }
      return false;
   }
   T &operator[](size_t idx)
   {
      if (idx >= size)
         throw std::out_of_range("Index out of range");
      return data[idx];
   }
   const T &operator[](size_t idx) const
   {
      if (idx >= size)
         throw std::out_of_range("Index out of range");
      return data[idx];
   }
   size_t getSize() const { return size; }
};

// 2. 复数类模板
template <typename T>
class Complex
{
private:
   T real, imag;

public:
   Complex(T r = 0, T i = 0) : real(r), imag(i) {}
   Complex operator+(const Complex &other) const
   {
      return Complex(real + other.real, imag + other.imag);
   }
   Complex operator-(const Complex &other) const
   {
      return Complex(real - other.real, imag - other.imag);
   }
   Complex operator*(const Complex &other) const
   {
      return Complex(real * other.real - imag * other.imag,
                     real * other.imag + imag * other.real);
   }
   Complex operator/(T scalar) const
   {
      return Complex(real / scalar, imag / scalar);
   }
   T magnitude() const
   {
      return std::sqrt(real * real + imag * imag);
   }
   ____qcodep____ // 输出Complex
};

// 3. 统计分析接口
template <typename T>
class Analyzer
{
public:
   virtual ~Analyzer() = default;
   virtual T mean() const = 0;
   virtual T variance() const = 0;
   virtual T max() const = 0;
   virtual std::string getType() const = 0;
   virtual const DataArray<T, 100> &getData() const = 0;
};

// 4. 数值数据分析器
template <typename T, size_t N>
class NumericAnalyzer : public Analyzer<T>
{
private:
   const DataArray<T, N> &data;

public:
   NumericAnalyzer(const DataArray<T, N> &d) : data(d) {}
   const DataArray<T, 100> &getData() const override
   {
      return reinterpret_cast<const DataArray<T, 100> &>(data);
   }
   T mean() const override
   {
      T sum = T();
      for (size_t i = 0; i < data.getSize(); i++)
      {
         sum += data[i];
      }
      return sum / static_cast<T>(data.getSize());
   }

   ____qcodep____ // variance函数

       T
       max() const override
   {
      if (data.getSize() == 0)
         throw std::runtime_error("Data is empty");
      T max_val = data[0];
      for (size_t i = 1; i < data.getSize(); i++)
      {
         if (data[i] > max_val)
            max_val = data[i];
      }
      return max_val;
   }
   std::string getType() const override { return "NumericAnalyzer"; }
};

// 5. 复数数据分析器
template <size_t N>
class ComplexAnalyzer : public Analyzer<Complex<double>>
{
private:
   const DataArray<Complex<double>, N> &data;

public:
   ____qcodep____ // 初始化函数

       const DataArray<Complex<double>, 100> &
       getData() const override
   {
      return reinterpret_cast<const DataArray<Complex<double>, 100> &>(data);
   }
   Complex<double> mean() const override
   {
      Complex<double> sum;
      for (size_t i = 0; i < data.getSize(); i++)
      {
         sum = sum + data[i];
      }
      return sum / static_cast<double>(data.getSize());
   }
   Complex<double> variance() const override{
       ____qcodep____ // 方差函数
   } Complex<double> max() const override
   {
      if (data.getSize() == 0)
         throw std::runtime_error("Data is empty");
      Complex<double> max_val = data[0];
      for (size_t i = 1; i < data.getSize(); i++)
      {
         if (data[i].magnitude() > max_val.magnitude())
         {
            max_val = data[i];
         }
      }
      return max_val;
   }
   std::string getType() const override { return "ComplexAnalyzer"; }
};

// 6. 数据处理框架
template <typename T, size_t N>
class DataProcessor
{
private:
   DataArray<T, N> data;
   std::unique_ptr<Analyzer<T>> analyzer;

public:
   void setAnalyzer(std::unique_ptr<Analyzer<T>> a) { analyzer = std::move(a); }

   bool addData(const T &val) { ____qcodep____ }

   void process() const
   {
      if (!analyzer)
         throw std::runtime_error("Analyzer not set");

      // 输出统计数据
      std::cout << "Mean: " << analyzer->mean() << "\n";
      std::cout << "Variance: " << analyzer->variance() << "\n";
      std::cout << "Max: " << analyzer->max() << "\n";

      // 输出原始数据
      std::cout << "Data:\n";
      const auto &dataArray = analyzer->getData();
      for (size_t i = 0; i < dataArray.getSize(); ++i)
      {
         std::cout << "Data[" << i << "]: " << dataArray[i] << "\n";
      }
   }
};

// 7. 主函数
int main()
{
   std::string dataType, structType;
   size_t dim;
   std::cin >> dataType;
   std::cin >> dim;

   if (dataType == "INT")
   {
      // 处理整数数据
      DataArray<int, 100> intData;
      int val;
      for (size_t i = 0; i < dim && std::cin >> val; ++i)
      {
         intData.push(val);
      }

      auto analyzer = std::make_unique<NumericAnalyzer<int, 100>>(intData);

      DataProcessor<int, 100> processor;
      processor.setAnalyzer(std::move(analyzer));

      for (size_t i = 0; i < intData.getSize(); ++i)
      {
         processor.addData(intData[i]);
      }

      processor.process();
   }
   else if (dataType == "COMPLEX")
   {
      // 处理复数数据
      DataArray<Complex<double>, 100> complexData;
      double real, imag;

      for (size_t i = 0; i < dim; ++i)
      {
         std::cin >> real >> imag;
         complexData.push(Complex<double>(real, imag));
      }

      auto analyzer = std::make_unique<ComplexAnalyzer<100>>(complexData);

      DataProcessor<Complex<double>, 100> processor;
      processor.setAnalyzer(std::move(analyzer));

      for (size_t i = 0; i < complexData.getSize(); ++i)
      {
         processor.addData(complexData[i]);
      }

      processor.process();
   }
   else
   {
      std::cerr << "Unsupported data type: " << dataType << std::endl;
      return 1;
   }

   return 0;
}