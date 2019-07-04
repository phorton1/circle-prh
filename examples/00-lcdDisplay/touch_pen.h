#ifndef __touch_pen__
#define __touch_pen__

#include <lcd/LCDWIKI_KBV.h> 


class touchPen
{
public:
    
    touchPen();
    ~touchPen() {}
    
    void task();
    
private:

    LCDWIKI_KBV my_lcd;
    
    void show_string(
        const char *str,
        int16_t x,
        int16_t y,
        uint8_t csize,
        uint16_t fc,
        uint16_t bc,
        boolean mode);
    
    void show_color_select_menu(void);
    void show_pen_size_select_menu(void);
    void show_main_menu(void);
    
    

    
    
    
    
    
};

#endif
