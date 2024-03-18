/********************************************************************************************************************************************
*                                                                                                                                           *
*              ---------------------------------以下是模块的修改记录区-----------------------------------------                             *
*                                                                                                                                           *
********************************************************************************************************************************************/
/**********************************************
 * 内容：
 * 日期：2024-02-18
 * 作者：YJX
 * 版本号：V1.0（初版）
 ***********************************************
 * 修改内容：
 * 修改日期：
 * 修改作者：
 * 版本号：
 ***********************************************
*/
/********************************************************************************************************************************************
*                                                                                                                                           *
*                ---------------------------------以下是模块的使用说明区-----------------------------------------                           *
*                                                                                                                                           *
********************************************************************************************************************************************/
/*                                       特点说明：
          【1】、使用Libusb库，在Linux系统下使用的USB打印机接口操作模块
          【2】、在Linux下需要在编译命令中增加 -lpthread 和  -lusb-1.0
*/
#include <libusb.h>//编译命令中增加 -lusb-1.0
#include <thread>//Linux需要在编译命令中增加 -lpthread
#include <iostream>//uint8_t的类型需要加上这个
#include "FUN_PrinterUSB.h"
/********************************************************************************************************************************************
*                                                                                                                                           *
*               ----------------------------------以下模块间的对接函数区-----------------------------------------                             *
*                                                                                                                                           *
********************************************************************************************************************************************/

/********************************************************************************************************************************************
*                                                                                                                                           *
*               ----------------------------------以下是模块间的变量申明区--------------------------------                            *
*                                                                                                                                           *
********************************************************************************************************************************************/

/********************************************************************************************************************************************
*                                                                                                                                           *
*               ----------------------------------以下是模块内的宏定义区------------------------------------                                  *
*                                                                                                                                           *
********************************************************************************************************************************************/
//打印机类的标准USB指令
#define PRINTER_GET_DEVICE_ID      0x00
#define PRINTER_GET_PORT_STATUS    0x01
#define PRINTER_SOFT_RESET         0x02
/********************************************************************************************************************************************
*                                                                                                                                           *
*               ----------------------------------以下是模块内的变量类型定义区------------------------------------                            *
*                                                                                                                                           *
********************************************************************************************************************************************/
//
static libusb_device_descriptor PrinterUSBDeviceDescriptor;//定义一个设备描述符结构体变量
static libusb_device *PrinterUSBDevice = NULL;
static libusb_device_handle *PrinterUSBDeviceHandle = NULL;//设备句柄
static int16_t PrinterEP_OUT,PrinterEP_IN;//端点地址(用uint8_t就可以，此处用int16_t是为了能够区分里面的值是否有效而定的，负数为无效值)
static uint16_t PrinterEP_OUT_SIZE,PrinterEP_IN_SIZE;//端点大小
//相关回调函数变量
static ReceCallBack PrinterUSB_ReceCallBack = NULL;//接收数据的回调函数指针
static RemoteCloseUSBCallBack PrinterUSB_CloseCallBack = NULL;//远程关闭USB回调函数指针

/********************************************************************************************************************************************
*                                                                                                                                           *
*               ----------------------------------以下是模块的内部函数申明区------------------------------------                            *
*                                                                                                                                           *
********************************************************************************************************************************************/
static void USBRece_threadMethod(int arc);
static int PrinterUSB_GetSubStr(uint8_t *datap,uint16_t datalen,uint8_t *SubStrNameP,uint8_t *SubStrValueP,uint16_t SubStrValueLen);
static int PrinterUSB_GET_DEVICE_ID(uint8_t *dp,uint16_t dl);
static char PrinterUSB_GET_DEVICE_STATUS(uint8_t *status);
static char PrinterUSB_RESET_DEVICE(void);

/********************************************************************************************************************************************
*                                                                                                                                           *
*               ----------------------------------以下是模块内的 变量 与 宏定义 申明区--------------------------------                            *
*                                                                                                                                           *
********************************************************************************************************************************************/

/********************************************************************************************************************************************
*                                                                                                                                           *
*               ----------------------------------以下是模块的系统函数代码区------------------------------------                            *
*                                                                                                                                           *
********************************************************************************************************************************************/

/********************************************************************************************************************************************
*                                                                                                                                           *
*               ----------------------------------以下是模块的用户函数代码区------------------------------------                            *
*                                                                                                                                           *
********************************************************************************************************************************************/
/**
 * @brief USB打印机开启函数
 * 
 * @param OpenPara 开启USB的参数
 * @return int8_t 返回0表示开启成功 小于0表示开启失败 
 */
char FUN_PrinterUSB_Open(OpenPrinterUSBParat *OpenPara)
{
     libusb_device **PrinterUSBDeviceS = NULL;
     libusb_config_descriptor *PrinterUSBConfig = NULL;
     int r,i,config;
     int altsetting_index;
     ssize_t cnt;
     

     //1、---------------------- libusb库初始化 -----------------------
     r = libusb_init(NULL);
     if(r < 0)
     {
          printf("\nFailed to initialise libusb\n");
          return -1;
     }

     //2、--------------------- 获取所有USB设备列表 -------------------------
     cnt = libusb_get_device_list(NULL, &PrinterUSBDeviceS);
     if (cnt < 0)
     {//列表数小于0，获取失败
          printf("\nThere are no USB devices on the bus\n");
          return -1;
     }

     //3、-------------------- 开始遍历USB设备列表 --------------------------
     r = 0;
     i = 0;
     while ((PrinterUSBDevice = PrinterUSBDeviceS[i++]) != NULL)
     {//遍历设备列表中的每个设备
          //获取设备描述符信息
          if(libusb_get_device_descriptor(PrinterUSBDevice, &PrinterUSBDeviceDescriptor) == 0)
          {//获取成功
               if(PrinterUSBDeviceDescriptor.idVendor == OpenPara->Vid && PrinterUSBDeviceDescriptor.idProduct == OpenPara->Pid)
               {//找到对应的pid,vid设备（注意，此处可能存在多个相同的pid,vid，此处只取第1个匹配的）
                    r = 1;
                    break;
               }
          }                  
     }//end of while


     if(r == 0)
     {//未找到
          printf("\nDevice NOT found\n");
          libusb_free_device_list(PrinterUSBDeviceS, 1);
          //libusb_close(handle);
          return -1;
     }

     //4、---------------------- 打开设备 ---------------------------
     r = libusb_open(PrinterUSBDevice, &PrinterUSBDeviceHandle);
     if (r < 0)
     {//打开设备失败
          
          printf("Error opening device,ECode = %d\n",r);//LIBUSB_ERROR_NOT_SUPPORTED);
          libusb_free_device_list(PrinterUSBDeviceS, 1);
          //libusb_close(handle);
          return -1;
     }


     //5、----------------------- 开始配置设备 --------------------------
     r = libusb_get_configuration(PrinterUSBDeviceHandle, &config);
     if(r)
     {
          printf("\n***Error in libusb_get_configuration\n");
          libusb_free_device_list(PrinterUSBDeviceS, 1);
          libusb_close(PrinterUSBDeviceHandle);
          return -1;
     }
     //printf("\nConfigured value: %d", config);//打印配置描述表的表数

     if(config != 1)
     {
          r = libusb_set_configuration(PrinterUSBDeviceHandle, 1);//使用1号配置值(如果要让设备处于未配置状态,则第2个参数为-1)
          if(r)
          {
               printf("Error in libusb_set_configuration\n");
               libusb_free_device_list(PrinterUSBDeviceS, 1);
               libusb_close(PrinterUSBDeviceHandle);
               return -1;
          }
     }

     libusb_free_device_list(PrinterUSBDeviceS, 1);


     //---------------------------------------
     //6、--------------------- 判断内核 与 驱动 是否绑定 ----------------------------------------
     if(libusb_kernel_driver_active(PrinterUSBDeviceHandle, 0) == 1)
     {//绑定
          //printf("\nKernel Driver Active");
          if(libusb_detach_kernel_driver(PrinterUSBDeviceHandle, 0))
          {//与内核 解绑失败
               printf("\nCouldn't detach kernel driver!\n");
               //libusb_free_device_list(PrinterUSBDeviceS, 1);
               libusb_close(PrinterUSBDeviceHandle);
               return -1;
          }
     }

     //7、------------------------------ 申明接口 ------------------------------------------
     r = libusb_claim_interface(PrinterUSBDeviceHandle, 0);
     if(r < 0)
     {
          printf("\nCannot Claim Interface");
          //libusb_free_device_list(PrinterUSBDeviceS, 1);
          libusb_close(PrinterUSBDeviceHandle);
          return -1;
     }

     //8、------------------- 获取端点描述信息，并获取收发端点号 --------------------------------------
     //获取端点描述信息
     //从设备PrinterUSBDevice获取配置描述符
     r = libusb_get_active_config_descriptor(PrinterUSBDevice, &PrinterUSBConfig);//获取当活动的配置描述信息
     if(r < 0)
     {
          printf("\nGet Donfig descriptor Fail");
          //libusb_free_device_list(PrinterUSBDeviceS, 1);
          libusb_close(PrinterUSBDeviceHandle);
          return -1;
     }

     // printf("\n------------------- Configure Descriptors -------------------");
     // printf("\nConfigure Descriptors: ");    
     // printf("\n\tLength: %d", PrinterUSBConfig->bLength);
     // printf("\n\tDesc_Type: %d", PrinterUSBConfig->bDescriptorType);
     // printf("\n\tTotal length: 0x%04x", PrinterUSBConfig->wTotalLength);
     // printf("\n\tNumber of Interfaces: %d", PrinterUSBConfig->bNumInterfaces);
     // printf("\n\tConfiguration Value: %d", PrinterUSBConfig->bConfigurationValue);
     // printf("\n\tConfiguration String Index: %d", PrinterUSBConfig->iConfiguration);
     // printf("\n\tConfiguration Attributes: 0x%02x", PrinterUSBConfig->bmAttributes);
     // printf("\n\tMaxPower(2mA): %d\n", PrinterUSBConfig->MaxPower);
     // printf("\n-------------------------------------------------------------");

     //从配置描述符获取接口描述符
     libusb_interface *iface = NULL;
     for (i = 0; i < PrinterUSBConfig->bNumInterfaces; i++)
     {//遍历该配置中的所有接口
          iface = (libusb_interface *)&PrinterUSBConfig->interface[i];//获取每个接口描述信息
          libusb_interface_descriptor *altsetting = NULL;//定义一个接口描述符指针
          for(altsetting_index = 0; altsetting_index < iface->num_altsetting; altsetting_index++)
          {//遍历接口的所有描述符
               altsetting = (libusb_interface_descriptor *)&iface->altsetting[altsetting_index];
                              
               // printf("\n------------------- Interface Descriptors(%d) -------------------",altsetting_index);
               // printf("\nInterface Descriptors: ");   
               // printf("\n\tbLength: %d", altsetting->bLength);
               // printf("\n\tbDescriptorType: %d",altsetting->bDescriptorType);
               // printf("\n\tbInterfaceNumber: %d",altsetting->bInterfaceNumber);
               // printf("\n\tbAlternateSetting: %d",altsetting->bAlternateSetting);
               // printf("\n\tbNumEndpoints: %d",altsetting->bNumEndpoints);
               // printf("\n\tiInterfaceClass: %d",altsetting->bInterfaceClass);
               // printf("\n\tiInterfaceSubClass: %d",altsetting->bInterfaceSubClass);
               // printf("\n\tbInterfaceProtocol: %d",altsetting->bInterfaceProtocol);
               // printf("\n\tiInterface: %d",altsetting->iInterface);
               // printf("\n-------------------------------------------------------------");

               if(altsetting->bInterfaceClass != LIBUSB_CLASS_PRINTER || altsetting->bInterfaceSubClass != 1)
               {//判断不是打印机类接口
                    //break;
                    continue;//以下代码不执行，继续遍历下一个接口描述符
               }

               int endpoint_index;
               PrinterEP_IN = -1;
               PrinterEP_OUT = -1;
               PrinterEP_OUT_SIZE = 0;
               PrinterEP_IN_SIZE = 0;
               //libusb_endpoint_desriptor *ep;
               for(endpoint_index=0; endpoint_index < altsetting->bNumEndpoints; endpoint_index++)
               {//遍历接口描述符中的所有端点
                    const struct libusb_endpoint_desriptor *ep = (const struct libusb_endpoint_desriptor *)&altsetting->endpoint[endpoint_index];
                    struct libusb_endpoint_descriptor *endpoint = (struct libusb_endpoint_descriptor *)ep;
                    //Out_epdesc = altsetting->endpoint[endpoint_index];

                    if(endpoint->bmAttributes & LIBUSB_TRANSFER_TYPE_BULK)
                    {//是批量传输
                         if(((endpoint->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT))
                         {//是OUT端点
                              PrinterEP_OUT = endpoint->bEndpointAddress;  
                              PrinterEP_OUT_SIZE = endpoint->wMaxPacketSize;
                         }
                         else if(((endpoint->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN))
                         {//是IN端点
                              PrinterEP_IN = endpoint->bEndpointAddress;
                              PrinterEP_IN_SIZE = endpoint->wMaxPacketSize;
                         }
                    }
                    
                    // printf("\n------------------- EndPoint Descriptors(%d) -------------------",endpoint_index);
                    // printf("\n\tEndPoint Descriptors: ");
                    // printf("\n\t\tSize of EndPoint Descriptor: %d", endpoint->bLength);
                    // printf("\n\t\tType of Descriptor: %d", endpoint->bDescriptorType);
                    // printf("\n\t\tEndpoint Address: 0x%02x", endpoint->bEndpointAddress);
                    // printf("\n\t\tAttributes applied to Endpoint: %d", endpoint->bmAttributes);
                    // printf("\n\t\tMaximum Packet Size: %d", endpoint->wMaxPacketSize);
                    // printf("\n\t\tInterval for Polling for data Tranfer: %d\n", endpoint->bInterval);
                    // printf("\n-------------------------------------------------------------");
               }  
               if(PrinterEP_IN > 0 && PrinterEP_OUT > 0)
               {
                    break;//退出遍历循环
               }    
          }
          
          if(PrinterEP_IN > 0 && PrinterEP_OUT > 0)
          {
               break;//退出遍历循环
          } 
     }


     //9、------------------------------------- 启动接收线程 ----------------------------------------
     PrinterUSB_ReceCallBack = OpenPara->ReceDataFun;
     PrinterUSB_CloseCallBack = OpenPara->RemoteCloseUSBCallBackFun;
     //创建接收线程t1，并运行
     std::thread t1(USBRece_threadMethod,1);//带有一个参数，目前未使用
     t1.detach();//将线程与主线程分离，主线程结束，线程随之结束。

     return 0;
}

/**
 * @brief 往USB打印机写数据的函数
 * 
 * @param data 待写数据的首地址
 * @param offset 数据的偏移字节量
 * @param len 待写数据的字节数
 * @return uint16_t 实际写入的字节量
 */
uint16_t FUN_PrinterUSB_Write(uint8_t *data, uint16_t offset, uint16_t len)
{
     int r, wlength = 0;
     if(len > PrinterEP_OUT_SIZE)
     {
          len = PrinterEP_OUT_SIZE;
     }
     r = libusb_bulk_transfer(PrinterUSBDeviceHandle, PrinterEP_OUT, (unsigned char *)(data + offset), len, &wlength, 0);
     if(r)
     {//发送异常
          return 0;
     }
     return (uint16_t)wlength;
}

/**
 * @brief 输出设备的字符串信息的函数
 * 
 * @param strinfop 存放字符串信息的结构体指针
 * @return char 小于0表示获取失败 0表示获取成功
 */
char FUN_PrinterUSB_OutStrInfo(DeviceStrInfo *strinfop)
{
     int r;
     unsigned char a[256];

     // //获取设备描述字符串信息---制造商
     // r = libusb_get_string_descriptor_ascii(PrinterUSBDeviceHandle, PrinterUSBDeviceDescriptor.iManufacturer, (unsigned char*) strinfop->Device_MFG, sizeof(((DeviceStrInfo*)0)->Device_MFG));
     // if (r < 0)
     // {
     //      return -1;
     // }

     // //获取设备描述字符串信息---产品
     // r = libusb_get_string_descriptor_ascii(PrinterUSBDeviceHandle, PrinterUSBDeviceDescriptor.iProduct, (unsigned char*) strinfop->Device_MDL, sizeof(((DeviceStrInfo*)0)->Device_MDL));
     // if(r < 0)
     // {
     //      return -1;
     // }

     strinfop->Device_MFG[0] = '\0';
     strinfop->Device_MDL[0] = '\0';
     strinfop->Device_SN[0] = '\0';

     //获取设备的MFG、MDL
     r = PrinterUSB_GET_DEVICE_ID(a,sizeof(a));
     if(r < 0)
     {
          return -1;
     }

     //获取MFG数据
     PrinterUSB_GetSubStr(a,r,(uint8_t *)"MFG",strinfop->Device_MFG,sizeof(((DeviceStrInfo*)0)->Device_MFG));
     //获取MDL数据
     PrinterUSB_GetSubStr(a,r,(uint8_t *)"MDL",strinfop->Device_MDL,sizeof(((DeviceStrInfo*)0)->Device_MDL));


     //获取设备描述字符串信息---序列号
     r = libusb_get_string_descriptor_ascii(PrinterUSBDeviceHandle, PrinterUSBDeviceDescriptor.iSerialNumber, (unsigned char*) strinfop->Device_SN, sizeof(((DeviceStrInfo*)0)->Device_SN));
     if(r < 0)
     {
          return -1;
     }     

     return 0;
}

/**
 * @brief 获取USB打印机的设备ID信息函数
 * 
 * @param sp 信息存放空间首地址
 * @param sl 信息存放空间字节数
 * @return short 有符号16位，小于0表示获取失败，大等于0表示有效字节数
 */
short FUN_PrinterUSB_Get_Device_ID(uint8_t *sp,uint16_t sl)
{
     return (short)PrinterUSB_GET_DEVICE_ID(sp,sl);
}

/**
 * @brief 获取USB打印机设备状态
 * 
 * @param status 用于存放状态的空间首地址
 * @return char 小于0表示获取失败 其他值表示有效字节数（本函数返回的有效字节数为1）
 */
char FUN_PrinterUSB_Get_Status(uint8_t *status)
{
     return PrinterUSB_GET_DEVICE_STATUS(status);
}

/**
 * @brief 发送标准的打印机复位指令
 * 
 * @return char 0:发送复位成功 小于0:发送复位失败
 */
char FUN_PrinterUSB_Reset(void)
{
     return PrinterUSB_RESET_DEVICE();
}

/**
 * @brief USB打印机关闭函数
 * 
 * @return int8_t 返回0表示关闭成功 小于0表示关闭失败
 */
char FUN_PrinterUSB_Close(void)
{
     libusb_release_interface(PrinterUSBDeviceHandle, 0);
     //libusb_free_device_list(PrinterUSBDeviceS, 1);
     libusb_close(PrinterUSBDeviceHandle);
     return 0;
}

/********************************************************************************************************************************************
*                                                                                                                                           *
*               ----------------------------------以下是模块的内部函数代码区------------------------------------                            *
*                                                                                                                                           *
********************************************************************************************************************************************/

//USB接收线程方法
static void USBRece_threadMethod(int arc)
{
     int rv,length;
     unsigned char a[1024 * 50];
     //std::this_thread::sleep_for(std::chrono::seconds(arc));//本线程休眠stime秒

     while(1)
     {
          length = 0;
          rv = libusb_bulk_transfer(PrinterUSBDeviceHandle,PrinterEP_IN,a,sizeof(a),&length,0);	
          if(rv < 0) 
          {
               if(rv == LIBUSB_ERROR_NO_DEVICE)
               {//设备关闭
                    PrinterUSB_CloseCallBack();                    
               }
               break;//退出循环
          }
          else if(rv == 0)
          {//读取成功
               PrinterUSB_ReceCallBack(a, length);
          }
     }
     
     libusb_release_interface(PrinterUSBDeviceHandle, 0);
     //libusb_free_device_list(PrinterUSBDeviceS, 1);
     libusb_close(PrinterUSBDeviceHandle);
}

//从设备ID指令返回的数据中，获取指定名称的字符信息
static int PrinterUSB_GetSubStr(uint8_t *datap,uint16_t datalen,uint8_t *SubStrNameP,uint8_t *SubStrValueP,uint16_t SubStrValueLen)
{
     uint8_t u8f;
     uint16_t u16i,u16f;

     SubStrValueP[0] = '\0';
     u8f = 0;//未找到标志
     u16f = 0;
     for(u16i = 0; u16i < datalen; u16i++)
     {
          if(u8f == 0)
          {//未找到
               if(datap[u16i] != SubStrNameP[u16f])
               {//不相等                    
                    if((SubStrNameP[u16f] == '\0') && (datap[u16i] == ':'))
                    {//找到了
                         u8f = 1;//找到
                    }
                    u16f = 0;
               }
               else
               {//相等
                    u16f++;
               }
          }
          else
          {//已找到
               if(datap[u16i] == ';')
               {
                    SubStrValueP[u16f] = '\0';
                    return u16f;//正常
               }
               else
               {
                    SubStrValueP[u16f++] = datap[u16i];
                    if(u16f >= SubStrValueLen)
                    {
                         SubStrValueP[u16f] = '\0';
                         return -1;//溢出
                    }
               }               
          }
     }
     return 0;//未找到
}

//执行一次获取设备ID的打印类的命令
static int PrinterUSB_GET_DEVICE_ID(uint8_t *dp,uint16_t dl)
{
     int rv;

     //以下语句相当于发送 0xA1 0x00 0x00 0x00 0x00 0x00 0x01 0x00
     rv = libusb_control_transfer(PrinterUSBDeviceHandle,LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,PRINTER_GET_DEVICE_ID,0x00,0x00,dp,dl,0);
     if(rv < 0) 
     {//出错了
          return -1;
     }
     return rv;
}

//执行一次获取设备状态的打印类命令
static char PrinterUSB_GET_DEVICE_STATUS(uint8_t *status)
{
     int rv;
     unsigned char a[10];

     //以下语句相当于发送 0xA1 0x01 0x00 0x00 0x00 0x00 0x01 0x00
     rv = libusb_control_transfer(PrinterUSBDeviceHandle,LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,PRINTER_GET_PORT_STATUS,0x00,0x00,a,sizeof(a),0);
     if(rv != 1) 
     {//出错了
          return -1;
     }
     *status = a[0];
     return 1;
}

//执行一次对设备进行复位的打印类命令
static char PrinterUSB_RESET_DEVICE(void)
{
     int rv;
     unsigned char a[10];

     //以下语句相当于发送 0x21 0x02 0x00 0x00 0x00 0x00 0x01 0x00
     rv = libusb_control_transfer(PrinterUSBDeviceHandle,LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,PRINTER_SOFT_RESET,0x00,0x00,a,sizeof(a),0);
     if(rv < 0) 
     {//出错了
          return -1;
     }
     return 0;
}
