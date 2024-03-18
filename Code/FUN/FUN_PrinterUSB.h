#ifndef FUN_PrinterUSB_h
#define FUN_PrinterUSB_h

#include <libusb.h>//编译命令中增加 -lusb-1.0
#include <iostream>//uint8_t的类型需要加上这个
#include <functional>//std::function需要用到的
#include <memory>//std::unique需要用到的
#include <thread>//Linux需要在编译命令中增加 -lpthread

typedef enum
{
     OperationSuccess = 0,//USB操作成功
     LibInitFail = -1,//库初始化失败
     NoUSBDevice = -2,//在Bus总线上没有USB设备
     USBNoFound = -3,//没有找到USB设备
     OpenFail = -4,//USB打开失败
     GetConfFail = -5,//获取配置描述符失败
     SetConfFail = -6,//设置配置描述符失败
     FreeKernelDriverFail = -7,//释放USB设备的内核驱动失败
     ClaimInterfaceFail = -8,//声明USB接口失败
     GetEndpointDescriptorFail = -9//获取端点描述符失败
}USBOperationResult;//USB操作结果

typedef std::function<void(uint8_t*, uint16_t)> USBReceDataEvent;//USB收到数据事件
typedef std::function<void()> USBCloseEvent;//USB关闭事件

typedef struct
{
     unsigned char Device_MFG[50];//设备产商
     unsigned char Device_MDL[50];//设备型号
     unsigned char Device_SN[50];//设备序列号
}DeviceStrInfo;

class Libusb_Printer
{
public:
     Libusb_Printer(USBReceDataEvent,USBCloseEvent);
     ~Libusb_Printer();
     //
     USBOperationResult FUN_PrinterUSB_Open(uint16_t pid,uint16_t vid);
     uint8_t FUN_PrinterUSB_OutStrInfo(DeviceStrInfo *strinfop);
     int16_t FUN_PrinterUSB_Get_Device_ID(uint8_t *sp,uint16_t sl);
     int8_t FUN_PrinterUSB_Get_Status(uint8_t  *status);
     int8_t FUN_PrinterUSB_Reset(void);
     uint16_t FUN_PrinterUSB_Send(uint8_t *data, uint16_t offset, uint16_t len);
     //
     USBOperationResult FUN_PrinterUSB_Close(void);


private:
     //接收线程方法申明
     void PrinterUSB_ReceThreadMethod(void);
     //从设备ID指令返回的数据中，获取指定名称的字符信息
     int PrinterUSB_GetSubStr(uint8_t *datap,uint16_t datalen,uint8_t *SubStrNameP,uint8_t *SubStrValueP,uint16_t SubStrValueLen);
     //执行一次获取设备ID的打印类的命令
     int PrinterUSB_GET_DEVICE_ID(uint8_t *dp,uint16_t dl);
     //执行一次获取设备状态的打印类命令
     int8_t PrinterUSB_GET_DEVICE_STATUS(uint8_t *status);
     //执行一次对设备进行复位的打印类命令
     int8_t PrinterUSB_RESET_DEVICE(void);
     //关闭USB
     void PrinterUSB_Close(void);
     //
     USBReceDataEvent ReceDataFun;//USB收到数据回调函数
     USBCloseEvent CloseFun;//USB关闭回调函数
     //智能指针管理 接收数据线程对象
     std::unique_ptr<std::thread> Rece_thread_ptr;
     //
     libusb_device *PrinterUSBDevice;
     libusb_device_handle *PrinterUSBDeviceHandle;//设备句柄
     libusb_device_descriptor PrinterUSBDeviceDescriptor;//定义一个设备描述符结构体变量
     int16_t PrinterEP_OUT,PrinterEP_IN;//端点地址(用uint8_t就可以，此处用int16_t是为了能够区分里面的值是否有效而定的，负数为无效值)
     uint16_t PrinterEP_OUT_SIZE,PrinterEP_IN_SIZE;//端点大小
     //
     uint8_t PrinterOpenFlag;//0:未开启 非0:开启
};


#endif
