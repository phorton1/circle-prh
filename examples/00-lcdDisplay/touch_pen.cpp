#define TEST_NO_RPI 1


#include "touch_pen.h"
#include "kernel.h"
//#include <lcd/TouchScreen.h> 
#include <lcd/LCDWIKI_GUI.h> 
#include <lcd/LCDWIKI_KBV.h> 
#include <lcd/_pins_arduino.h>

#define log_name "touch_pen"
#include "../../_pins_arduino.h"

#define LCD_RST     8
#define LCD_CS      9
#define LCD_CD      10
#define LCD_WR      11
#define LCD_RD      12

#define YP          LCD_CS  
#define XM          LCD_CD  
#define YM          1       
#define XP          0       

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

uint16_t color_mask[] = {0xF800,0xFFE0,0x07E0,0x07FF,0x001F,0xF81F}; //color select

#define COLORBOXSIZE my_lcd.Get_Display_Width()/6
#define PENBOXSIZE   my_lcd.Get_Display_Width()/4

uint16_t old_color, current_color,flag_colour;
uint16_t old_pen,current_pen,flag_pen;
boolean show_flag = true;


//-------------------------------------------
// ctor == setup()
//-------------------------------------------

touchPen::touchPen() :
    my_lcd(ILI9486, LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RST)
{
    LOG("touchPen starting",0);
    my_lcd.Init_LCD();
    printf("touchPen() lcd ID=%04x\n",my_lcd.Read_ID());
    
    my_lcd.Fill_Screen(BLACK);
    show_main_menu();
    current_color = RED;
    current_pen = 0;
    delay(200);
    
    #if TEST_NO_RPI
        printf("magic pattern ...\n");
        pinMode(LCD_WR,OUTPUT);
        pinMode(LCD_RD,OUTPUT);
        digitalWrite(LCD_WR,LOW);
        digitalWrite(LCD_RD,LOW);
        pinMode(YP,INPUT);
        pinMode(XM,INPUT);
        pinMode(YM,INPUT);
        pinMode(XP,INPUT);
    #endif

    delay(200);
    printf("started ...\n\n");
}



//-----------------------------------
// app stuff
//-----------------------------------

void touchPen::show_string(
    const char *str,
    int16_t x,
    int16_t y,
    uint8_t csize,
    uint16_t fc,
    uint16_t bc,
    boolean mode)
{
    my_lcd.Set_Text_Mode(mode);
    my_lcd.Set_Text_Size(csize);
    my_lcd.Set_Text_colour(fc);
    my_lcd.Set_Text_Back_colour(bc);
    my_lcd.Print_String(str,x,y);
}


void touchPen::show_color_select_menu(void)
{
    uint16_t i;
    for(i = 0;i<6;i++)
    {
        my_lcd.Set_Draw_color(color_mask[i]);
        my_lcd.Fill_Rectangle(i*COLORBOXSIZE, 0, (i+1)*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
    }  
    my_lcd.Set_Draw_color(GREEN);
    my_lcd.Fill_Round_Rectangle(
        (my_lcd.Get_Display_Width()-20)/3+10,
        COLORBOXSIZE/2+2,
        (my_lcd.Get_Display_Width()-20)/3*2+10,
        COLORBOXSIZE/2+20,
        5);
    show_string("OK",CENTER,COLORBOXSIZE/2+4,2,RED, BLACK,1);
}


void touchPen::show_pen_size_select_menu(void)
{
    uint16_t i;
    my_lcd.Set_Text_Mode(1);
    my_lcd.Set_Text_Size(2);
    my_lcd.Set_Text_colour(GREEN);
    my_lcd.Set_Text_Back_colour(BLACK);
    for(i = 0;i<4;i++)
    {
        my_lcd.Print_Number_Int(
            i+1,
            5+PENBOXSIZE*i,
            (COLORBOXSIZE/2-16)/2,
            0,
            ' ',
            10);
        my_lcd.Set_Draw_color(RED);
        my_lcd.Fill_Rectangle(
            25+PENBOXSIZE*i,
            COLORBOXSIZE/2/2-i,
            PENBOXSIZE*(i+1)-10,
            COLORBOXSIZE/2/2+i);
    }
    my_lcd.Set_Draw_color(GREEN);
    my_lcd.Fill_Round_Rectangle(
        (my_lcd.Get_Display_Width()-20)/3+10,
        COLORBOXSIZE/2+2,
        (my_lcd.Get_Display_Width()-20)/3*2+10,
        COLORBOXSIZE/2+20,
        5);
    show_string("OK",CENTER,COLORBOXSIZE/2+4,2,RED, BLACK,1);
}


void touchPen::show_main_menu(void)
{
    my_lcd.Set_Draw_color(YELLOW);
    my_lcd.Fill_Round_Rectangle(
        5,
        0,
        (my_lcd.Get_Display_Width()-20)/3+5,
        COLORBOXSIZE/2+20,
        5);
    my_lcd.Fill_Round_Rectangle(
        (my_lcd.Get_Display_Width()-20)/3*2+15,
        0,
        (my_lcd.Get_Display_Width()-20)/3*3+15,
        COLORBOXSIZE/2+20,
        5);
    my_lcd.Set_Draw_color(MAGENTA);
    my_lcd.Fill_Round_Rectangle(
        (my_lcd.Get_Display_Width()-20)/3+10,
        0,
        (my_lcd.Get_Display_Width()-20)/3*2+10,
        COLORBOXSIZE/2+20,
        5);
    show_string("COLOUR",
        5+((my_lcd.Get_Display_Width()-20)/3-72)/2-1,
        ((COLORBOXSIZE/2+20)-16)/2,
        2, BLUE, BLACK, 1);
    show_string("CLEAR",
        (my_lcd.Get_Display_Width()-20)/3+10+((my_lcd.Get_Display_Width()-20)/3-60)/2-1,
        ((COLORBOXSIZE/2+20)-16)/2,
        2, WHITE, BLACK, 1);
    show_string("PENSIZE",
        (my_lcd.Get_Display_Width()-20)/3*2+15+((my_lcd.Get_Display_Width()-20)/3-84)/2-1,
        ((COLORBOXSIZE/2+20)-16)/2,
        2, BLUE, BLACK, 1);
}




//-------------------------------------------
// task == loop()
//-------------------------------------------

void touchPen::task()
{
   
#if !TEST_NO_RPI

    static u32 poll_time = 0;
    u32 now = CTimer::Get()->GetTicks();    // ms
    if (now > poll_time + 200)
    {
        static int count = 0;
        printf("polling arduino(%d) ...\n",count++);
        poll_time = now;
        
        // set all the pins to input to let arduino have em
        // and tell the arduino to start the measurment
        
        setReadDir();
        pinMode(YP,INPUT);
        pinMode(XM,INPUT);
        pinMode(YM,INPUT);
        pinMode(XP,INPUT);
        digitalWrite(LCD_WR,LOW);
        digitalWrite(LCD_RD,LOW);
        CTimer::Get()->usDelay(20);
        
        pinMode(LCD_WR,INPUT);
        pinMode(LCD_RD,INPUT);
        CTimer::Get()->usDelay(10);
        
        // wait for RD to go high
        
        while (!digitalRead(LCD_WR))
        {
            CTimer::Get()->usDelay(1);
        }
        
        // wait for WR to go low
        
        while (digitalRead(LCD_WR))
        {
            CTimer::Get()->usDelay(2);
        }

        // read the result from the arduino
        
        printf("got result\n");
        
        // return the system to it's normal state
        // (all outputs), idle
        
        setWriteDir();
        pinMode(YP,OUTPUT);
        pinMode(XM,OUTPUT);
        pinMode(YM,OUTPUT);
        pinMode(XP,OUTPUT);
        pinMode(LCD_CS,OUTPUT);
        pinMode(LCD_CD,OUTPUT);
        pinMode(LCD_WR,OUTPUT);
        pinMode(LCD_RD,OUTPUT);
        digitalWrite(LCD_WR,HIGH);
        digitalWrite(LCD_RD,HIGH);
        
        printf("finished arduino poll\n");
        
        poll_time = CTimer::Get()->GetTicks();
    }

#endif


    
#if 0
    
again:
    // digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    // digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) 
    {
        if (p.y < (TS_MINY-5)) 
        {
            my_lcd.Set_Draw_color(BLACK);
            my_lcd.Fill_Rectangle(0, COLORBOXSIZE, my_lcd.Get_Display_Width()-1, my_lcd.Get_Display_Height()-1);
        }

        //p.x = my_lcd.Get_Display_Width()-map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
        //p.y = my_lcd.Get_Display_Height()-map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(), 0);
        
        p.x = map(p.x, TS_MINX, TS_MAXX, my_lcd.Get_Display_Width(), 0);
        p.y = map(p.y, TS_MINY, TS_MAXY, my_lcd.Get_Display_Height(),0);
        if (p.y < COLORBOXSIZE/2+20) 
        {
            //select color

            if (((p.x>5)&&(p.x < ((my_lcd.Get_Display_Width()-20)/3+5)))&&!flag_pen)
            {
                flag_colour = 1;
                if (show_flag)
                {
                    my_lcd.Set_Draw_color(BLACK);
                    my_lcd.Fill_Rectangle(0,0,my_lcd.Get_Display_Width()-1,COLORBOXSIZE/2+20);
                    show_color_select_menu();
                }
                show_flag = false;
                switch (current_color)
                {
                    case RED:
                    {
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(0, 0, COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                        break;  
                    }
                    case YELLOW:
                    {
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(COLORBOXSIZE, 0, 2*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                        break; 
                    }
                    case GREEN:
                    {
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(2*COLORBOXSIZE, 0, 3*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                        break; 
                    }
                    case CYAN:
                    {
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(3*COLORBOXSIZE, 0, 4*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                        break;                 
                    }
                    case BLUE:
                    {
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(4*COLORBOXSIZE, 0, 5*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                        break;   
                    }
                    case MAGENTA:  
                    {
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(5*COLORBOXSIZE, 0, 6*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                        break;   
                    }
                    default:
                        break;
                }
            }
            
            if (flag_colour)
            {
                if (p.y < COLORBOXSIZE/2)
                {
                    old_color = current_color;
                    if (p.x < COLORBOXSIZE) 
                    { 
                        current_color = RED; 
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(0, 0, COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                    } 
                    else if (p.x < COLORBOXSIZE*2) 
                    {
                        current_color = YELLOW;
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(COLORBOXSIZE, 0, 2*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                    } 
                    else if (p.x < COLORBOXSIZE*3) 
                    {
                        current_color = GREEN;
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(2*COLORBOXSIZE, 0, 3*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                    } 
                    else if (p.x < COLORBOXSIZE*4) 
                    {
                        current_color = CYAN;
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(3*COLORBOXSIZE, 0, 4*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                    } 
                    else if (p.x < COLORBOXSIZE*5) 
                    {
                        current_color = BLUE;
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(4*COLORBOXSIZE, 0, 5*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                    } 
                    else if (p.x < COLORBOXSIZE*6) 
                    {
                        current_color = MAGENTA;
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(5*COLORBOXSIZE, 0, 6*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                    }
                    if (old_color != current_color)
                    {
                        switch(old_color)
                        {
                            case RED:
                            {
                                my_lcd.Set_Draw_color(RED);
                                my_lcd.Draw_Rectangle(0, 0, COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                                break;  
                            }
                            case YELLOW:
                            {
                                my_lcd.Set_Draw_color(YELLOW);
                                my_lcd.Draw_Rectangle(COLORBOXSIZE, 0, 2*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                                break; 
                            }
                            case GREEN:
                            {
                                my_lcd.Set_Draw_color(GREEN);
                                my_lcd.Draw_Rectangle(2*COLORBOXSIZE, 0, 3*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                                break; 
                            }
                            case CYAN:
                            {
                                my_lcd.Set_Draw_color(CYAN);
                                my_lcd.Draw_Rectangle(3*COLORBOXSIZE, 0, 4*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                                break;                 
                            }
                            case BLUE:
                            {
                                my_lcd.Set_Draw_color(BLUE);
                                my_lcd.Draw_Rectangle(4*COLORBOXSIZE, 0, 5*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                                break;   
                            }
                            case MAGENTA:  
                            {
                                my_lcd.Set_Draw_color(MAGENTA);
                                my_lcd.Draw_Rectangle(5*COLORBOXSIZE, 0, 6*COLORBOXSIZE-1, COLORBOXSIZE/2-1);
                                break;   
                            }
                            default:
                                break;                        
                        }
                    }
                }
                else if (p.y < COLORBOXSIZE/2+20)
                {
                    if ((p.x>(my_lcd.Get_Display_Width()-20)/3+10)&&(p.x<(my_lcd.Get_Display_Width()-20)/3*2+10))
                    {
                        my_lcd.Set_Draw_color(BLACK);
                        my_lcd.Fill_Rectangle(0,0,my_lcd.Get_Display_Width()-1,COLORBOXSIZE/2+20);
                        show_main_menu();
                        flag_colour = 0;
                        show_flag = true;
                        goto again;
                    }
                }
            }

            //select pen size

            if (((p.x>((my_lcd.Get_Display_Width()-20)/3*2+15))&&(p.x < (((my_lcd.Get_Display_Width()-20)/3*3+15))))&&!flag_colour)
            {
                flag_pen = 1;
                if (show_flag)
                {
                    my_lcd.Set_Draw_color(BLACK);
                    my_lcd.Fill_Rectangle(0,0,my_lcd.Get_Display_Width()-1,COLORBOXSIZE/2+20);
                    show_pen_size_select_menu();
                }
                show_flag = false;
                switch(current_pen)
                {
                    case 0:
                    {
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(0, 0, PENBOXSIZE-1, COLORBOXSIZE/2-1);
                        break;
                    }
                    case 1:
                    {
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(PENBOXSIZE, 0, 2*PENBOXSIZE-1, COLORBOXSIZE/2-1);
                        break; 
                    }
                    case 2:
                    {
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(2*PENBOXSIZE, 0, 3*PENBOXSIZE-1, COLORBOXSIZE/2-1);
                        break;   
                    }
                    case 3:  
                    {
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(3*PENBOXSIZE, 0, 4*PENBOXSIZE-1, COLORBOXSIZE/2-1);
                        break;                
                    }
                    default:
                        break;
                }              
            }
            
            if (flag_pen)
            {
                if (p.y < COLORBOXSIZE/2)
                {
                    old_pen = current_pen;
                    if (p.x < PENBOXSIZE)
                    {
                        current_pen = 0;
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(0, 0, PENBOXSIZE-1, COLORBOXSIZE/2-1);
                    }
                    else if (p.x < 2*PENBOXSIZE)
                    {
                        current_pen = 1;
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(PENBOXSIZE, 0, 2*PENBOXSIZE-1, COLORBOXSIZE/2-1);
                    }
                    else if (p.x < 3*PENBOXSIZE) 
                    {
                        current_pen = 2;
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(2*PENBOXSIZE, 0, 3*PENBOXSIZE-1, COLORBOXSIZE/2-1);
                    }
                    else if (p.x < 4*PENBOXSIZE)
                    {
                        current_pen = 3;
                        my_lcd.Set_Draw_color(WHITE);
                        my_lcd.Draw_Rectangle(3*PENBOXSIZE, 0, 4*PENBOXSIZE-1, COLORBOXSIZE/2-1);               
                    }
                    if (old_pen != current_pen)
                    {
                        switch(old_pen)
                        {
                            case 0:
                            {
                                my_lcd.Set_Draw_color(BLACK);
                                my_lcd.Draw_Rectangle(0, 0, PENBOXSIZE-1, COLORBOXSIZE/2-1);
                                break;
                            }
                            case 1:
                            {
                                my_lcd.Set_Draw_color(BLACK);
                                my_lcd.Draw_Rectangle(PENBOXSIZE, 0, 2*PENBOXSIZE-1, COLORBOXSIZE/2-1);
                                break; 
                            }
                            case 2:
                            {
                                my_lcd.Set_Draw_color(BLACK);
                                my_lcd.Draw_Rectangle(2*PENBOXSIZE, 0, 3*PENBOXSIZE-1, COLORBOXSIZE/2-1);
                                break;   
                            }
                            case 3:  
                            {
                                my_lcd.Set_Draw_color(BLACK);
                                my_lcd.Draw_Rectangle(3*PENBOXSIZE, 0, 4*PENBOXSIZE-1, COLORBOXSIZE/2-1);
                                break;                
                            }
                            default:
                                break;           
                        }      
                    }
                }
                else if (p.y < COLORBOXSIZE/2+20)
                {
                    if ((p.x>(my_lcd.Get_Display_Width()-20)/3+10)&&(p.x<(my_lcd.Get_Display_Width()-20)/3*2+10))
                    {
                        my_lcd.Set_Draw_color(BLACK);
                        my_lcd.Fill_Rectangle(0,0,my_lcd.Get_Display_Width()-1,COLORBOXSIZE/2+20);
                        show_main_menu();
                        flag_pen = 0;
                        show_flag = true;
                        goto again;
                    }  
                }
            }
            
            if (((p.x>((my_lcd.Get_Display_Width()-20)/3+10))&&(p.x < ((my_lcd.Get_Display_Width()-20)/3*2+10)))&&!flag_colour&&!flag_pen)
            {
                my_lcd.Set_Draw_color(BLACK);  
                my_lcd.Fill_Rectangle(0,COLORBOXSIZE,my_lcd.Get_Display_Width()-1,my_lcd.Get_Display_Height()-1);
            }
        }
        if (((p.y-current_pen) > COLORBOXSIZE/2+20) && ((p.y+current_pen) < my_lcd.Get_Display_Height()))  //drawing
        {
            my_lcd.Set_Draw_color(current_color);
            // if (1 == current_pen)
            //   {
            //      my_lcd.Draw_Pixel(p.x,  p.y);
            //   }
            //   else 
            //   {
            my_lcd.Fill_Circle(p.x,  p.y,current_pen);
            //  }
        }
    }
#endif    
}

