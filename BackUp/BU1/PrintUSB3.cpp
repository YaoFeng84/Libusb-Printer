#include "FUN_PrinterUSB.h"
#include "Cal.h"
#include <thread>

//extern short AddCal(short n1,short n2);

#define USB_VENDOR_ID 0xD23C
#define USB_PRODUCT_ID 0x0001

//接收数据回调函数
void ReceDataCallback(uint8_t *data,uint16_t len)
{
     printf("Rece length = %d,Rece data = 0x%02x\n",len,data[0]);
     return;
}

void RemoteCloseUSBCallback(void)
{
     printf("Remote Close USB\n");
}

int main(void)
{
     OpenPrinterUSBParat openpara;
     openpara.Pid = USB_PRODUCT_ID;
     openpara.Vid = USB_VENDOR_ID;
     openpara.ReceDataFun = ReceDataCallback;
     openpara.RemoteCloseUSBCallBackFun = RemoteCloseUSBCallback;
     if(FUN_PrinterUSB_Open(&openpara))
     {//设备未找到
          printf("Device no found\n");
     }
     else
     {//设备已找到
     
     //打印机发送数据测试
          uint8_t buf[] = "PRINT 1,1\r\nGET Speed\r\n";
          printf("Send data = %s\n",buf);
          printf("Send data bytenum = %d\n",FUN_PrinterUSB_Write(buf,0,sizeof(buf)));//发送数据

     //获取打印机信息测试
          DeviceStrInfo dsi;
          if(FUN_PrinterUSB_OutStrInfo(&dsi))//获取设备信息
          {
               printf("Get Device Info Fail\n");
          }
          else
          {
               printf("MFG = %s\nMDL = %s\nSN = %s\n",dsi.Device_MFG,dsi.Device_MDL,dsi.Device_SN);
          }

     //读取打印机状态测试
          if(FUN_PrinterUSB_Get_Status(buf) == 1)
          {
               printf("status = 0x%02x\n",buf[0]);
          }
          else
          {
               printf("Get Printer Status Fail\n");
          }

     //计算
          printf("n1 + n2 = %d\n",AddCal(1,2));
          std::this_thread::sleep_for(std::chrono::seconds(5));//延时5秒等待接收完成后再关闭

          FUN_PrinterUSB_Close();//关闭设备
          printf("Device closed\n");
     }
     system("pause");
     return 0;
}


