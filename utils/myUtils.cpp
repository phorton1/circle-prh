#include <circle/devicenameservice.h>
#include <circle/koptions.h>
#include <circle/device.h>
#include <circle/timer.h>
#include <circle/util.h>


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
    printf("%-12s  ",s);
    char buf[17];
    memset(buf,0,17);
    for (int i=0; i<len; i++)
    {
        if (i && i%16==0)
        {
            printf("   %s\n%-12s  ",buf,"");
            memset(buf,0,17);
        }
        buf[i % 16] = p[i]>' ' ? p[i] : '.';
        printf("%02x ",p[i]);
    }
    while (len % 16 != 0)
    {
        printf("   ");
        len++;
    }
    printf("   %s\n",buf);
}    


