#include <iostream>
#include <vector>
#include <ctime>
#include <iomanip>
using namespace std;

struct DeviceLog
{
   time_t timestamp;
   string event;

   void showLog() const
   {
      cout << event << endl;
   }
};

class SmartDevice
{
public:
   // 构造函数
   SmartDevice(const string &id, const string &name)
       : ____qcodep____
   {
      totalDevices++;
      this->id = id;
      this->name = name;
      isOnline = false;
      powerLevel = 0;
      addLog("Device created");
   }

   // 拷贝构造函数(深拷贝)
   SmartDevice(const SmartDevice &other)
       : ____qcodep____
   { // 深拷贝日志
      totalDevices++;
      id = other.id + "_copy";
      name = other.name;
      isOnline = other.isOnline;
      powerLevel = other.powerLevel;
      ____qcodep____

          addLog("Device copied from " + other.id);

      cout << "=== The copy constructor is called ===" << endl;
      cout << "From device " << other.id << " create new device " << id << endl;
   }

   // 析构函数
   ~SmartDevice()
   {
      totalDevices--;
      if (isOnline)
      {
         ____qcodep____
             addLog("Device destroyed while running");
      }
      else
      {
         addLog("Device destroyed");
      }

      cout << "=== The destructor is called ===" << endl;
      cout << "device " << id << " is destroyed" << endl;
      cout << "The number of remaining devices: " << totalDevices << endl;
      cout << "Equipment in operation: " << runningDevices << endl;

      delete logs;
   }

   // 设备上线
   void powerOn()
   {
      if (!isOnline)
      {
         isOnline = true;
         powerLevel = 1;
         runningDevices++;
         addLog("Device powered on");
      }
   }

   // 设备下线
   void powerOff()
   {
      if (isOnline)
      {
         isOnline = false;
         powerLevel = 0;
         runningDevices--;
         addLog("Device powered off");
      }
   }

   // 调整功率
   void setPowerLevel(int level)
   {
      if (isOnline && level >= 1 && level <= 5)
      {
         powerLevel = level;
         addLog("Power level set to " + to_string(level));
      }
   }

   // 显示设备信息
   void showInfo() const
   {
      cout << "Device ID: " << id << endl;
      cout << "Device Name: " << name << endl;
      cout << "Status: " << (isOnline ? "Online" : "Offline") << endl;
      if (isOnline)
      {
         cout << "Power Level: ";
         cout << " (" << powerLevel << "/5)" << endl;
      }
   }

   // 显示日志
   void showLogs() const
   {
      cout << "=== Device Logs ===" << endl;
      for (const auto &log : *logs)
      {
         log.showLog();
      }
   }

   // 静态成员函数
   static int getTotalDevices() { return totalDevices; }
   static int getRunningDevices() { return runningDevices; }

private:
   // 添加日志
   void addLog(const string &event){
       ____qcodep____}

   string id;
   string name;
   unsigned int isOnline : 1;   // 位域:0或1
   unsigned int powerLevel : 3; // 位域:0-5 (3位足够)

   vector<DeviceLog> *logs; // 设备日志

   static int totalDevices;   // 总设备数
   static int runningDevices; // 运行中设备数
};

// 初始化静态成员
int SmartDevice::totalDevices = 0;
int SmartDevice::runningDevices = 0;

// test函数
void testDeviceSystem()
{
   cout << "===== test1:create device =====" << endl;
   SmartDevice light1("L-1001", "Living Room Light");
   light1.showInfo();
   cout << "Total number of devices: " << SmartDevice::getTotalDevices() << endl;
   cout << "Equipment in operation: " << SmartDevice::getRunningDevices() << endl
        << endl;

   cout << "===== test2:operate device =====" << endl;
   light1.powerOn();
   light1.setPowerLevel(3);
   light1.showInfo();
   cout << "Total number of devices: " << SmartDevice::getTotalDevices() << endl;
   cout << "Equipment in operation: " << SmartDevice::getRunningDevices() << endl
        << endl;

   cout << "===== test3:copy device =====" << endl;
   SmartDevice light2 = light1;
   light2.showInfo();
   cout << "Total number of devices: " << SmartDevice::getTotalDevices() << endl;
   cout << "Equipment in operation: " << SmartDevice::getRunningDevices() << endl
        << endl;

   cout << "===== test4:create device dynamically =====" << endl;
   SmartDevice *ac = new SmartDevice("AC-2001", "Bedroom AC");
   ac->powerOn();
   ac->setPowerLevel(4);
   ac->showInfo();
   cout << "Total number of devices: " << SmartDevice::getTotalDevices() << endl;
   cout << "Equipment in operation: " << SmartDevice::getRunningDevices() << endl
        << endl;

   cout << "===== test5:show log =====" << endl;
   light1.showLogs();
   cout << endl;
   light2.showLogs();
   cout << endl;
   ac->showLogs();
   cout << endl;

   cout << "===== test6:turn off device =====" << endl;
   ac->powerOff();
   ac->showInfo();
   cout << "Total number of devices: " << SmartDevice::getTotalDevices() << endl;
   cout << "Equipment in operation: " << SmartDevice::getRunningDevices() << endl
        << endl;

   cout << "===== test7:Delete the dynamic device =====" << endl;
   delete ac;
   cout << "Total number of devices: " << SmartDevice::getTotalDevices() << endl;
   cout << "Equipment in operation: " << SmartDevice::getRunningDevices() << endl
        << endl;
}

int main()
{
   testDeviceSystem();
   cout << "===== over =====" << endl;
   cout << "Finally, total number of devices: " << SmartDevice::getTotalDevices() << endl;
   cout << "Finally, device in operation: " << SmartDevice::getRunningDevices();
   return 0;
}