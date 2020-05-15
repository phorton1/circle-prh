#include <utils/myUtils.h>
#include <circle/devicenameservice.h>
#include <circle/koptions.h>
#include <circle/device.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/memorymap.h>

#include <circle/logger.h>
#define log_name "dprobe"

// from map listing

extern unsigned char _etext;
extern unsigned char __init_start;
extern unsigned char __init_end;
extern unsigned char __exidx_start;
extern unsigned char __exidx_end;
extern unsigned char __bss_start;
extern unsigned char _end;

// from alloc.cpp

bool check_alloc();
void dump_alloc();




int dprobe_count = 0;
u32 kernel_checksum = 0;


void dlog(const char *f, ...);
void dumpAll();
bool checkAll();
u32  checkSumKernel();


void do_dprobe(int l, const char *f, ...)
{
    bool dump = checkAll();
    
    if (l || dump)
    {
        va_list var;
        va_start(var, f);
        
        CString s1;
        s1.FormatV(f, var);
        
        dlog("%s",(const char *)s1);
        
        if (l >= 2 || dump)
            dumpAll();
    }
    dprobe_count++;
}



bool checkAll()
{
    bool bad = false;
    u32 chk = checkSumKernel();

    if (!kernel_checksum)
        kernel_checksum = chk;
    else if (chk != kernel_checksum)
    {
        dlog("KERNEL CHECKSUM CHANGED !!!",0);
        bad = true;    
    }
    
    if (check_alloc())
    {
        dlog("PROBLEM IN alloc.cpp !!!",0);
        bad = true;    
    }
    
    return bad;
}



void dumpAll()
{
    dlog("kernel start=%08X bss=%08x end=%08x check=%08x",
         MEM_KERNEL_START,
         (u32)&__bss_start,
         (u32)&_end,kernel_checksum);
    dlog("_etext=%08x __init_start=%08x __init_end=%08x __exidx_start=%08x __exidx_end=%08x",
        (u32) &_etext,
        (u32) &__init_start,
        (u32) &__init_end,
        (u32) &__exidx_start,
        (u32) &__exidx_end);

    dump_alloc();
}





u32 checkSumKernel()
{
    u32 chk = 0;
    u32 *p = (u32 *) MEM_KERNEL_START;
    while (p < (u32 *)&__init_end)
        chk += *p++;
    return chk;
}



void dlog(const char *f, ...)
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
            va_start(var, f);
            
            CString s1;
            s1.FormatV(f, var);
            
            CString s2;
            s2.Format("\x1b[1mdprobe(%d): %s\n", dprobe_count, (const char *)s1);
            pDevice->Write((const char *) s2, s2.GetLength() );

            va_end(var);
        }
    } 
}

