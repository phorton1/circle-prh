#ifndef _STATUSSCREENH_
#define _STATUSSCREENH_

#include "std_kernel.h"
#include <circle/screen.h>


extern void print_screen(const char *pMessage, ...);


class statusScreen
{
public:

    statusScreen(CScreenDevice *pScreen);
    void init();
    void update();
    
private:
    
    bool initialized;
    
    CScreenDevice *screen;
    void cursor(int x, int y);
    
};


#endif // _STATUSSCREENH_