#include "main.h"

void putc(FrameBuffer *fb, char chara)
{
    char *str = nullptr;
    if (chara == '\r')
    {
        str[0] = '\n';
        str[1] = '\0';
    }
    else if (chara == '\b')
    {
        str[0] = '\b';
        str[1] = ' ';
        str[2] = '\b';
        str[3] = '\0';
    }
    else
    {
        str[0] = chara;
        str[1] = '\0';
    }
    puts_font(fb, str);
}

void puts(FrameBuffer *fb, char *str)
{
    puts_font(fb, str);
}

void puts_font(FrameBuffer *fb, char *str)
{
    int basex = 1, basey = 1;
    for (; *str != '\0'; ++str)
    {
        if (*str == '\n'){
            basex = 1;
            basey += 19;
            continue;
        }
        for (int y = 0; y < 16; ++y)
        {
            for (int x = 0; x < 8; ++x)
            {
                if (font_map[*str][y][x])
                {
                    draw_pixel(basex + x, basey + y, Color_White, fb);
                }
            }
        }
        basex += 9;
    }
}