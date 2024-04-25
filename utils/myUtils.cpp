#include <circle/devicenameservice.h>
#include <circle/koptions.h>
#include <circle/device.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/util.h>
#include "myUtils.h"


void delay(unsigned int ms)
{
	CTimer *timer = CTimer::Get();
    if (timer)
        timer->MsDelay(ms);
}


void printf(const char *pMessage, ...)
{
    CDeviceNameService *ns = CDeviceNameService::Get();
    CKernelOptions *ko = CKernelOptions::Get();
    if (ns && ko)
    {
        CDevice *pDevice = ns->GetDevice(ko->GetLogDevice(), FALSE);        
        if (!pDevice)
            pDevice = ns->GetDevice("tty1", FALSE);        
        if (!pDevice)
            pDevice = ns->GetDevice("ttyS1", FALSE);
        if (!pDevice)
            pDevice = ns->GetDevice("ttyS2", FALSE);                   
        if (pDevice)
        {
            va_list var;
            va_start(var, pMessage);
            CString Message;
            Message.FormatV(pMessage, var);
            unsigned int len1 = pDevice->Write("\x1b[96m",5);		// light cyan
			assert(len1 == 5);
            unsigned int len2 = pDevice->Write((const char *) Message, Message.GetLength() );
			assert(len2 == Message.GetLength());
            va_end(var);
        }
    }
}


void display_bytes(const char *s, const unsigned char *p, int len)
{
	CString buf1;
	CString buf2;
	CString bbb;

	#define log_name ""
	
	buf1.Format("%-12s  ",s);
    for (int i=0; i<len; i++)
    {
        if (i && i%16==0)
        {
			buf1.Append(buf2);
			LOG("%s",(const char *) buf1);
			buf1.Format("%-12s  ","");
			buf2 = "";
        }

        bbb.Format("%02x ",p[i]);
		buf1.Append(bbb);
		bbb.Format("%c",p[i]>' ' ? p[i] : '.');
		buf2.Append(bbb);
    }
    while (len % 16 != 0)
    {
        buf1.Append("   ");
        len++;
    }
	buf1.Append(buf2);
	LOG("%s",(const char *) buf1);
}    


