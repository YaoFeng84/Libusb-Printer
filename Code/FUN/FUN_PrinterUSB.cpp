#include <FUN_PrinterUSB.h>

#define ReceBufferSize   (50 * 1024)    //接收缓存字节数

//打印机类的标准USB指令
#define PRINTER_GET_DEVICE_ID      0x00
#define PRINTER_GET_PORT_STATUS    0x01
#define PRINTER_SOFT_RESET         0x02


Libusb_Printer::Libusb_Printer(USBReceDataEvent urdfun,USBCloseEvent ucfun):
     ReceDataFun(urdfun),
     CloseFun(ucfun),
     Rece_thread_ptr(nullptr),
     PrinterUSBDevice(NULL),
     PrinterUSBDeviceHandle(NULL),
     PrinterEP_OUT(-1),
     PrinterEP_IN(-1),
     PrinterOpenFlag(0)
{

}

Libusb_Printer::~Libusb_Printer()
{

}



USBOperationResult Libusb_Printer::FUN_PrinterUSB_Open(uint16_t pid,uint16_t vid)
{
     libusb_device **PrinterUSBDeviceS = NULL;
     libusb_config_descriptor *PrinterUSBConfig = NULL;
     int r,i,config;
     int altsetting_index;
     ssize_t cnt;
     
     Libusb_Printer::PrinterUSB_Close();

     //1、---------------------- libusb库初始化 -----------------------
     r = libusb_init(NULL);
     if(r < 0)
     {
          //printf("\nFailed to initialise libusb\n");
          return LibInitFail;
     }

     //2、--------------------- 获取所有USB设备列表 -------------------------
     cnt = libusb_get_device_list(NULL, &PrinterUSBDeviceS);
     if (cnt < 0)
     {//列表数小于0，获取失败
          //printf("\nThere are no USB devices on the bus\n");
          return NoUSBDevice;
     }

     //3、-------------------- 开始遍历USB设备列表 --------------------------
     r = 0;
     i = 0;
     while ((PrinterUSBDevice = PrinterUSBDeviceS[i++]) != NULL)
     {//遍历设备列表中的每个设备
          //获取设备描述符信息
          if(libusb_get_device_descriptor(PrinterUSBDevice, &PrinterUSBDeviceDescriptor) == 0)
          {//获取成功
               if(PrinterUSBDeviceDescriptor.idVendor == vid && PrinterUSBDeviceDescriptor.idProduct == pid)
               {//找到对应的pid,vid设备（注意，此处可能存在多个相同的pid,vid，此处只取第1个匹配的）
                    r = 1;
                    break;
               }
          }                  
     }//end of while


     if(r == 0)
     {//未找到
          //printf("\nDevice NOT found\n");
          libusb_free_device_list(PrinterUSBDeviceS, 1);
          //libusb_close(handle);
          return USBNoFound;
     }

     //4、---------------------- 打开设备 ---------------------------
     r = libusb_open(PrinterUSBDevice, &PrinterUSBDeviceHandle);
     if (r < 0)
     {//打开设备失败
          
          //printf("Error opening device,ECode = %d\n",r);//LIBUSB_ERROR_NOT_SUPPORTED);
          libusb_free_device_list(PrinterUSBDeviceS, 1);
          //libusb_close(handle);
          return OpenFail;
     }


     //5、----------------------- 开始配置设备 --------------------------
     r = libusb_get_configuration(PrinterUSBDeviceHandle, &config);
     if(r)
     {
          //printf("\n***Error in libusb_get_configuration\n");
          libusb_free_device_list(PrinterUSBDeviceS, 1);
          libusb_close(PrinterUSBDeviceHandle);
          return GetConfFail;
     }
     //printf("\nConfigured value: %d", config);//打印配置描述表的表数

     if(config != 1)
     {
          r = libusb_set_configuration(PrinterUSBDeviceHandle, 1);//使用1号配置值(如果要让设备处于未配置状态,则第2个参数为-1)
          if(r)
          {
               //printf("Error in libusb_set_configuration\n");
               libusb_free_device_list(PrinterUSBDeviceS, 1);
               libusb_close(PrinterUSBDeviceHandle);
               return SetConfFail;
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
               //printf("\nCouldn't detach kernel driver!\n");
               //libusb_free_device_list(PrinterUSBDeviceS, 1);
               libusb_close(PrinterUSBDeviceHandle);
               return FreeKernelDriverFail;
          }
     }

     //7、------------------------------ 申明接口 ------------------------------------------
     r = libusb_claim_interface(PrinterUSBDeviceHandle, 0);
     if(r < 0)
     {
          //printf("\nCannot Claim Interface");
          //libusb_free_device_list(PrinterUSBDeviceS, 1);
          libusb_close(PrinterUSBDeviceHandle);
          return ClaimInterfaceFail;
     }

     //8、------------------- 获取端点描述信息，并获取收发端点号 --------------------------------------
     //获取端点描述信息
     //从设备PrinterUSBDevice获取配置描述符
     r = libusb_get_active_config_descriptor(PrinterUSBDevice, &PrinterUSBConfig);//获取当活动的配置描述信息
     if(r < 0)
     {
          //printf("\nGet Donfig descriptor Fail");
          //libusb_free_device_list(PrinterUSBDeviceS, 1);
          libusb_close(PrinterUSBDeviceHandle);
          return GetEndpointDescriptorFail;
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
     PrinterOpenFlag = 1;
     Rece_thread_ptr = std::make_unique<std::thread>(&Libusb_Printer::PrinterUSB_ReceThreadMethod,this);//
     Rece_thread_ptr->detach();//分离线程与主线程，主线程结束，线程随之结束。

     return OperationSuccess;
}

uint8_t Libusb_Printer::FUN_PrinterUSB_OutStrInfo(DeviceStrInfo *strinfop)
{
     int r;
     unsigned char a[256];

     if(PrinterOpenFlag == 0)
     {//未开启
          return -1;
     } 

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
     r = Libusb_Printer::PrinterUSB_GET_DEVICE_ID(a,sizeof(a));
     if(r < 0)
     {
          return -1;
     }

     //获取MFG数据
     Libusb_Printer::PrinterUSB_GetSubStr(a,r,(uint8_t *)"MFG",strinfop->Device_MFG,sizeof(((DeviceStrInfo*)0)->Device_MFG));
     //获取MDL数据
     Libusb_Printer::PrinterUSB_GetSubStr(a,r,(uint8_t *)"MDL",strinfop->Device_MDL,sizeof(((DeviceStrInfo*)0)->Device_MDL));


     //获取设备描述字符串信息---序列号
     r = libusb_get_string_descriptor_ascii(PrinterUSBDeviceHandle, PrinterUSBDeviceDescriptor.iSerialNumber, (unsigned char*) strinfop->Device_SN, sizeof(((DeviceStrInfo*)0)->Device_SN));
     if(r < 0)
     {
          return -1;
     }     

     return 0;
}

int16_t Libusb_Printer::FUN_PrinterUSB_Get_Device_ID(uint8_t *sp,uint16_t sl)
{
     if(PrinterOpenFlag == 0)
     {
          return -1;
     }
     return (int16_t)PrinterUSB_GET_DEVICE_ID(sp,sl);
}

int8_t Libusb_Printer::FUN_PrinterUSB_Get_Status(uint8_t  *status)
{
     if(PrinterOpenFlag == 0)
     {
          return -1;
     }
     return PrinterUSB_GET_DEVICE_STATUS(status);
}

int8_t Libusb_Printer::FUN_PrinterUSB_Reset(void)
{
     if(PrinterOpenFlag == 0)
     {
          return -1;
     }
     return PrinterUSB_RESET_DEVICE();
}

uint16_t Libusb_Printer::FUN_PrinterUSB_Send(uint8_t *data, uint16_t offset, uint16_t len)
{
     int r, wlength = 0;

     if(PrinterOpenFlag == 0)
     {
          return 0;
     }

     if(len > PrinterEP_OUT_SIZE)
     {
          len = PrinterEP_OUT_SIZE;
     }
     r = libusb_bulk_transfer(PrinterUSBDeviceHandle, PrinterEP_OUT, (unsigned char *)(data + offset), len, &wlength, 0);
     if(r)
     {//发送异常
          wlength = 0;
     }    
     return (uint16_t)wlength;
}

USBOperationResult Libusb_Printer::FUN_PrinterUSB_Close(void)
{
     Libusb_Printer::PrinterUSB_Close();
     return OperationSuccess;
}




//接收数据线程方法
void Libusb_Printer::PrinterUSB_ReceThreadMethod(void)
{
     int rv,length;
     unsigned char a[ReceBufferSize];
     //std::this_thread::sleep_for(std::chrono::seconds(arc));//本线程休眠stime秒

     while(1)
     {
          length = 0;
          rv = libusb_bulk_transfer(PrinterUSBDeviceHandle,PrinterEP_IN,a,sizeof(a),&length,0);	
          // std::cout<<"rvrvrv = "<<std::to_string(rv)<<std::endl;
          if(rv < 0) 
          {
               // if(rv == LIBUSB_ERROR_NO_DEVICE)
               // {//设备关闭
               CloseFun();                    
               // }
               break;//退出循环
          }
          else if(rv == 0)
          {//读取成功
               ReceDataFun(a, length);
          }
     }

     Libusb_Printer::PrinterUSB_Close();         
}

int Libusb_Printer::PrinterUSB_GetSubStr(uint8_t *datap,uint16_t datalen,uint8_t *SubStrNameP,uint8_t *SubStrValueP,uint16_t SubStrValueLen)
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
int Libusb_Printer::PrinterUSB_GET_DEVICE_ID(uint8_t *dp,uint16_t dl)
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
int8_t Libusb_Printer::PrinterUSB_GET_DEVICE_STATUS(uint8_t *status)
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
int8_t Libusb_Printer::PrinterUSB_RESET_DEVICE(void)
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


void Libusb_Printer::PrinterUSB_Close(void)
{
     if(PrinterOpenFlag)
     {
          PrinterOpenFlag = 0;
          try
          {
               libusb_release_interface(PrinterUSBDeviceHandle, 0);
          }
          catch(...){} 
          //libusb_free_device_list(PrinterUSBDeviceS, 1);
          try
          {
               libusb_close(PrinterUSBDeviceHandle);
          }
          catch(...){}
     }
     PrinterEP_OUT = -1;
     PrinterEP_IN = -1;
}


