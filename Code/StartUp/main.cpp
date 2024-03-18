#include "FUN_PrinterUSB.h"

#define USB_VENDOR_ID 0xD23C
#define USB_PRODUCT_ID 0x0001

std::unique_ptr<Libusb_Printer> Printer_ptr;

//接收数据回调函数
void ReceDataCallback(uint8_t *data,uint16_t len)
{
     // printf("Rece length = %d,Rece data = 0x%02x\n",len,data[0]);
     std::cout<<"Rece length = "<<len<<",Rece data = 0x"<<std::hex<<(int)data[0]<<std::endl;
     return;
}

void RemoteCloseUSBCallback(void)
{
     // printf("Remote Close USB\n");
     std::cout<<"Remote Close USB"<<std::endl;
}

int main(int argc,char *argv[])
{
     int16_t int16temp = 0;
     Printer_ptr = std::make_unique<Libusb_Printer>(ReceDataCallback,RemoteCloseUSBCallback);
     int16temp = (int16_t)Printer_ptr->FUN_PrinterUSB_Open(USB_PRODUCT_ID,USB_VENDOR_ID);
     
     if(int16temp)
     {//开启失败
          std::cout<<"Open USB Fail, Error Code = "<<std::to_string(int16temp)<<std::endl;
     }
     else
     {//开启成功
          //发送数据
          uint8_t buf[] = "PRINT 1,1\r\nGET Speed\r\n";
          int16temp = Printer_ptr->FUN_PrinterUSB_Send(buf,0,sizeof(buf));
          std::cout<<"Send length = "<<std::to_string(int16temp)<<std::endl;

          //获取打印机信息测试
          DeviceStrInfo dsi;
          if(Printer_ptr->FUN_PrinterUSB_OutStrInfo(&dsi))//获取设备信息
          {
               // printf("Get Device Info Fail\n");
               std::cout<<"Get Device Info Fail"<<std::endl;
          }
          else
          {
               // printf("MFG = %s\nMDL = %s\nSN = %s\n",dsi.Device_MFG,dsi.Device_MDL,dsi.Device_SN);
               std::cout<<"MFG = "<<dsi.Device_MFG<<std::endl;
               std::cout<<"MDL = "<<dsi.Device_MDL<<std::endl;
               std::cout<<"SN = "<<dsi.Device_SN<<std::endl;
          }

          //读取打印机状态测试
          if(Printer_ptr->FUN_PrinterUSB_Get_Status(buf) == 1)
          {
               // printf("status = 0x%02x\n",buf[0]);
               std::cout<<"status = 0x"<<std::hex<<(int)buf[0]<<std::endl;
          }
          else
          {
               // printf("Get Printer Status Fail\n");
               std::cout<<"Get Printer Status Fail"<<std::endl;
          }

     }     
     
     //
     std::this_thread::sleep_for(std::chrono::seconds(2));
     // std::cout<<"Close USB"<<std::endl;
     Printer_ptr->FUN_PrinterUSB_Close();
     return 0;
}


