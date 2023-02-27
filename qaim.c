/*
--------------------------------------------------
    James William Fletcher (github.com/mrbid)
--------------------------------------------------
        FEBRUARY 2023
        ~~~~~~~~~~~~~

    Quake Champions Trigger Bot

    Set enemy outline to "Target" and enemy outline color to red.
    
    Prereq:
    sudo apt install clang xterm espeak libx11-dev libxdo-dev libespeak-dev

*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <X11/Xutil.h>
#include <sys/stat.h>

#include <xdo.h>
#include <espeak/speak_lib.h>

#pragma GCC diagnostic ignored "-Wgnu-folding-constant"
#pragma GCC diagnostic ignored "-Wunused-result"

#define SCAN_DELAY 1000
#define uint uint
uint sd = 28;
uint sd2 = 14;
uint sps = 0;

Display *d;
int si;
Window twin;
Window this_win = 0;
uint x=0, y=0;

uint minimal = 0;
uint enable = 0;
uint crosshair = 1;
uint hotkeys = 1;


/***************************************************
   ~~ Utils
*/
int key_is_pressed(KeySym ks)
{
    // https://www.cl.cam.ac.uk/~mgk25/ucs/keysymdef.h
    // https://stackoverflow.com/questions/18281412/check-keypress-in-c-on-linux/52801588
    char keys_return[32];
    XQueryKeymap(d, keys_return);
    KeyCode kc2 = XKeysymToKeycode(d, ks);
    int isPressed = !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));
    return isPressed;
}
uint64_t microtime()
{
    struct timeval tv;
    struct timezone tz;
    memset(&tz, 0, sizeof(struct timezone));
    gettimeofday(&tv, &tz);
    return 1000000 * tv.tv_sec + tv.tv_usec;
}
uint espeak_fail = 0;
void speakS(const char* text)
{
    if(espeak_fail == 1)
    {
        char s[256];
        sprintf(s, "/usr/bin/espeak \"%s\"", text);
        system(s);
        usleep(33000);
    }
    else
    {
        espeak_Synth(text, strlen(text), 0, 0, 0, espeakCHARS_AUTO, NULL, NULL);
    }
}

/***************************************************
   ~~ X11 Utils
*/
Window getWindow(Display* d, const int si) // gets child window mouse is over
{
    XEvent event;
    memset(&event, 0x00, sizeof(event));
    XQueryPointer(d, RootWindow(d, si), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    event.xbutton.subwindow = event.xbutton.window;
    while(event.xbutton.subwindow)
    {
        event.xbutton.window = event.xbutton.subwindow;
        XQueryPointer(d, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    }
    return event.xbutton.window;
}
Window findWindow(Display *d, Window current, char const *needle)
{
    Window ret = 0, root, parent, *children;
    unsigned cc;
    char *name = NULL;

    if(current == 0)
        current = XDefaultRootWindow(d);

    if(XFetchName(d, current, &name) > 0)
    {
        if(strstr(name, needle) != NULL)
        {
            XFree(name);
            return current;
        }
        XFree(name);
    }

    if(XQueryTree(d, current, &root, &parent, &children, &cc) != 0)
    {
        for(uint i = 0; i < cc; ++i)
        {
            Window win = findWindow(d, children[i], needle);

            if(win != 0)
            {
                ret = win;
                break;
            }
        }
        XFree(children);
    }
    return ret;
}
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD    1
#define _NET_WM_STATE_TOGGLE 2 
Bool MakeAlwaysOnTop(Display* display, Window root, Window mywin)
{
    // https://stackoverflow.com/a/16235920
    Atom wmStateAbove = XInternAtom(display, "_NET_WM_STATE_ABOVE", 1);
    if(wmStateAbove == None){return False;}
    Atom wmNetWmState = XInternAtom(display, "_NET_WM_STATE", 1);
    if(wmNetWmState == None){return False;}
    if(wmStateAbove != None)
    {
        XClientMessageEvent xclient;
        memset(&xclient, 0, sizeof(xclient));
        xclient.type = ClientMessage;
        xclient.window = mywin;
        xclient.message_type = wmNetWmState;
        xclient.format = 32;
        xclient.data.l[0] = _NET_WM_STATE_ADD;
        xclient.data.l[1] = wmStateAbove;
        xclient.data.l[2] = 0;
        xclient.data.l[3] = 0;
        xclient.data.l[4] = 0;
        XSendEvent(display, root, False, SubstructureRedirectMask|SubstructureNotifyMask, (XEvent*)&xclient);
        XFlush(display);
        return True;
    }
    return False;
}
Window getNextChild(Display* d, Window current)
{
    uint cc = 0;
    Window root, parent, *children;
    if(XQueryTree(d, current, &root, &parent, &children, &cc) == 0)
        return current;
    const Window rw = children[0];
    XFree(children);
    //printf("%lX\n", children[i]);
    return rw;
}
uint isEnemy()
{
    // get image block
    XImage *img = XGetImage(d, twin, x-sd2, y-sd2, sd, sd, AllPlanes, XYPixmap);
    if(img == NULL)
        return 0;

    // extract colour information
    int i = 0;
    for(int y = 0; y < sd; y++)
    {
        for(int x = 0; x < sd; x++)
        {
            const unsigned long pixel = XGetPixel(img, x, y);
            const unsigned char cr = (pixel & img->red_mask) >> 16;
            const unsigned char cg = (pixel & img->green_mask) >> 8;
            const unsigned char cb = pixel & img->blue_mask;

            //if(cr > 230 && cg < 13 && cb < 13)
            if(cr > 220 && cg < 33 && cb < 33)
            {
                // done
                XDestroyImage(img);
                sps++;
                return 1;
            }
            
            i++;
        }
    }

    // done
    XDestroyImage(img);
    sps++;
    return 0;
}

/***************************************************
   ~~ Program Entry Point
*/
void rainbow_line_printf(const char* text)
{
    static unsigned char base_clr = 0;
    if(base_clr == 0)
        base_clr = (rand()%125)+55;
    
    printf("\e[38;5;%im", base_clr);
    base_clr++;
    if(base_clr >= 230)
        base_clr = (rand()%125)+55;

    const uint len = strlen(text);
    for(uint i = 0; i < len; i++)
        printf("%c", text[i]);
    printf("\e[38;5;123m");
}
void reprint()
{
    system("clear");

    if(minimal == 0)
    {
        printf("\033[1m\033[0;31m>>> RedStrog Triggerbot v1 by MegaStrog <<<\e[0m\n");
        rainbow_line_printf("original code by the loser below\n");
        rainbow_line_printf("James Cuckiam Fletcher (github.com/mrbid)\n\n");
        rainbow_line_printf("L-CTRL + L-ALT = Toggle BOT ON/OFF\n");
        rainbow_line_printf("R-CTRL + R-ALT = Toggle HOTKEYS ON/OFF\n");
        rainbow_line_printf("F = Flip scan area size.\n");
        rainbow_line_printf("P = Toggle crosshair.\n");
        rainbow_line_printf("H = Hold pressed to print scans per second.\n");
        rainbow_line_printf("\nDisable the game crosshair.\n");
        rainbow_line_printf("> If your monitor provides a crosshair that will work fine.\n");
        rainbow_line_printf("> OR just use the crosshair this bot provides.\n");
        printf("\e[38;5;76m");
        printf("\nSet enemy outline to \"Target\" and outline color to red.\n\n");
        printf("\e[38;5;123m");

        if(twin != 0)
        {
            printf("Quake Champions Win: 0x%lX\n\n", twin);

            if(enable == 1)
                rainbow_line_printf("BOT: \033[1m\e[32mON\e[0m\n");
            else
                rainbow_line_printf("BOT: \033[1m\e[31mOFF\e[0m\n");

            if(crosshair > 0)
                rainbow_line_printf("CROSSHAIR: \033[1m\e[32mON\e[0m\n");
            else
                rainbow_line_printf("CROSSHAIR: \033[1m\e[31mOFF\e[0m\n");

            if(hotkeys == 1)
                rainbow_line_printf("HOTKEYS: \033[1m\e[32mON\e[0m\n");
            else
                rainbow_line_printf("HOTKEYS: \033[1m\e[31mOFF\e[0m\n");

            printf("\n");
        }
    }
    else
    {
        if(twin != 0)
        {
            if(enable == 1)
                printf("\e[38;5;%umBOT: \033[1m\e[32mON\e[0m | ", minimal);
            else
                printf("\e[38;5;%umBOT: \033[1m\e[31mOFF\e[0m | ", minimal);

            if(crosshair > 0)
                printf("\e[38;5;%umCROSSHAIR: \033[1m\e[32mON\e[0m | ", minimal);
            else
                printf("\e[38;5;%umCROSSHAIR: \033[1m\e[31mOFF\e[0m | ", minimal);

            if(hotkeys == 1)
                printf("\e[38;5;%umHOTKEYS: \033[1m\e[32mON\e[0m ", minimal);
            else
                printf("\e[38;5;%umHOTKEYS: \033[1m\e[31mOFF\e[0m ", minimal);

            fflush(stdout);
        }
        else
        {
            printf("\e[38;5;123mPress \e[38;5;76mL-CTRL\e[38;5;123m + \e[38;5;76mL-ALT\e[38;5;123m to enable bot.\e[0m");
            fflush(stdout);
        }
    }
}
int main(int argc, char *argv[])
{
    srand(time(0));

    // is minimal ui?
    if(argc == 2)
    {
        minimal = atoi(argv[1]);
        if(minimal == 1){minimal = 8;}
        else if(minimal == 8){minimal = 1;}
    }

    // init espeak
    if(espeak_Initialize(AUDIO_OUTPUT_SYNCH_PLAYBACK, 0, 0, 0) < 0)
        espeak_fail = 1;

    // intro
    reprint();

    //

    xdo_t* xdo;
    XColor c[9];
    GC gc = 0;
    time_t ct = time(0);

    //

    // try to open the default display
    d = XOpenDisplay(getenv("DISPLAY")); // explicit attempt on environment variable
    if(d == NULL)
    {
        d = XOpenDisplay((char*)NULL); // implicit attempt on environment variable
        if(d == NULL)
        {
            d = XOpenDisplay(":0"); // hedge a guess
            if(d == NULL)
            {
                printf("Failed to open display :'(\n");
                return 0;
            }
        }
    }

    // get default screen
    si = XDefaultScreen(d);

    //xdo
    xdo = xdo_new(":0");
    if(minimal > 0)
    {
        xdo_t* xdo;
        xdo = xdo_new_with_opened_display(d, NULL, 0);
        xdo_get_active_window(xdo, &this_win);
        xdo_set_window_property(xdo, this_win, "WM_NAME", "RedStrogV1");
        xdo_set_window_size(xdo, this_win, 600, 1, 0);
        MakeAlwaysOnTop(d, XDefaultRootWindow(d), this_win);
    }

    // get graphics context
    gc = DefaultGC(d, si);

    // find bottom window
    twin = findWindow(d, 0, "Quake Champions");
    if(twin != 0)
        reprint();
    
    while(1)
    {
        // loop every 1 ms (1,000 microsecond = 1 millisecond)
        usleep(SCAN_DELAY);

        // inputs
        if(key_is_pressed(XK_Control_L) && key_is_pressed(XK_Alt_L))
        {
            if(enable == 0)
            {                
                // get window
                twin = findWindow(d, 0, "Quake Champions");
                if(twin != 0)
                    twin = getNextChild(d, twin);
                else
                    twin = getWindow(d, si);

                if(twin == 0)
                {
                    if(minimal == 0)
                    {
                        printf("Failed to detect a Quake Champions window.\n");
                    }
                    else
                    {
                        system("clear");
                        printf("Failed to detect a Quake Champions window.");
                        fflush(stdout);
                    }
                }

                // get center window point (x & y)
                XWindowAttributes attr;
                XGetWindowAttributes(d, twin, &attr);
                x = attr.width/2;
                y = attr.height/2;

                // toggle
                enable = 1;
                usleep(300000);
                reprint();
                speakS("on");
            }
            else
            {
                enable = 0;
                usleep(300000);
                reprint();
                speakS("off");
            }
        }
        
        // bot on/off
        if(enable == 1)
        {
            // always tracks sps
            static uint64_t st = 0;
            if(microtime() - st >= 1000000)
            {
                if(key_is_pressed(XK_H))
                {
                    if(minimal > 0){system("clear");}
                    printf("\e[36mSPS: %u\e[0m", sps);
                    if(minimal == 0){printf("\n");}else{fflush(stdout);}
                }
                sps = 0;
                st = microtime();
            }

            // input toggle
            if(key_is_pressed(XK_Control_R) && key_is_pressed(XK_Alt_R))
            {
                if(hotkeys == 0)
                {
                    hotkeys = 1;
                    usleep(300000);
                    reprint();
                    speakS("hk on");
                }
                else
                {
                    hotkeys = 0;
                    usleep(300000);
                    reprint();
                    speakS("hk off");
                }
            }

            if(hotkeys == 1)
            {
                // crosshair toggle
                if(key_is_pressed(XK_P))
                {
                    if(crosshair == 0)
                    {
                        crosshair = 1;
                        usleep(300000);
                        reprint();
                        speakS("cx on");
                    }
                    else
                    {
                        crosshair = 0;
                        usleep(300000);
                        reprint();
                        speakS("cx off");
                    }
                }

                // scan size flippening
                if(key_is_pressed(XK_F))
                {
                    if(sd == 28)
                        sd = 14;
                    else
                        sd = 28;
                    sd2 = sd/2;
                    usleep(300000);
                }
            }

            if(isEnemy() == 1)
            {
                xdo_mouse_down(xdo, CURRENTWINDOW, 1);
                usleep(100000); // or cheating ban
                xdo_mouse_up(xdo, CURRENTWINDOW, 1);

                // display ~1s recharge time
                if(crosshair != 0)
                {
                    crosshair = 2;
                    ct = time(0);
                }
            }

            if(crosshair == 1)
            {
                XSetForeground(d, gc, 65280);
                XDrawRectangle(d, twin, gc, x-sd2-1, y-sd2-1, sd+2, sd+2);
                XFlush(d);
            }
            else if(crosshair == 2)
            {
                if(time(0) > ct+1)
                    crosshair = 1;

                XSetForeground(d, gc, 16711680);
                XDrawRectangle(d, twin, gc, x-sd2-1, y-sd2-1, sd+2, sd+2);
                XSetForeground(d, gc, 65280);
                XDrawRectangle(d, twin, gc, x-sd2-2, y-sd2-2, sd+4, sd+4);
                XFlush(d);
            }
        }

        //
    }

    // done, never gets here in regular execution flow
    XCloseDisplay(d);
    return 0;
}

