#ifndef FUN_PrinterUSB111_h
#define FUN_PrinterUSB111_h


typedef void(*ReceCallBack)(unsigned char*, unsigned short);//收到数据回调函数
typedef void(*RemoteCloseUSBCallBack)(void);//设备断开USB回调函数
//
typedef struct
{
     unsigned short Pid;//uint16_t
     unsigned short Vid;
     ReceCallBack ReceDataFun;
     RemoteCloseUSBCallBack RemoteCloseUSBCallBackFun;
}OpenPrinterUSBParat;
//
typedef struct
{
     unsigned char Device_MFG[50];//设备产商
     unsigned char Device_MDL[50];//设备型号
     unsigned char Device_SN[50];//设备序列号
}DeviceStrInfo;

extern char FUN_PrinterUSB_Open(OpenPrinterUSBParat *OpenPara);
extern unsigned short FUN_PrinterUSB_Write(unsigned char *data, unsigned short offset, unsigned short len);
extern char FUN_PrinterUSB_OutStrInfo(DeviceStrInfo *strinfop);
extern short FUN_PrinterUSB_Get_Device_ID(unsigned char *sp,unsigned short  sl);
extern char FUN_PrinterUSB_Get_Status(unsigned char  *status);
extern char FUN_PrinterUSB_Reset(void);
extern char FUN_PrinterUSB_Close(void);

#endif
