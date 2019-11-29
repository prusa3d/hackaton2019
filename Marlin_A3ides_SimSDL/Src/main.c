// main.c

#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "stm32f4xx_hal.h"

#include "sim_st7789v.h"
#include "sdl_helper.h"

#include "jogwheel.h"
#include "guitypes.h"

const char* appname = "Marlin-A3ides SimSDL";

#define JOGW_PLUS 1
#define JOGW_MINUS 2
#define JOGW_BUTTON 4

#define JOGW_AR_PAUSE 300
#define JOGW_AR_DELAY 50

int jogw_flags = 0;

clock_t timer_loop = 0;
clock_t timer_loop_old = 0;
clock_t timer_loop_delta = 0;

SPI_HandleTypeDef hspi2;

void draw_test1(SDL_Surface* surface)
{
    SDL_FillRect(surface, NULL, 0);
}

void draw_test2(SDL_Surface* surface)
{
    //		sim_st7789v_pixels = (uint32_t*)surface->pixels;
    for (int n = 0; n < (ST7789_COLS * ST7789_ROWS); n++) {
        float x = (float)sim_st7789v_x / ST7789_COLS;
        float y = (float)sim_st7789v_y / ST7789_ROWS;
        uint32_t r = x * (1 << 5);
        uint32_t g = x * (1 << 6);
        uint32_t b = y * (1 << 5);
        sim_st7789v_wr_565(((r & 0x1f) << 11) | ((g & 0x3f) << 5) | (b & 0x1f));
        /*			int x = rand() % ST7789_COLS;
				int y = rand() % ST7789_ROWS;
				uint32_t pixel = 0x00000000;//rand() * 100000;
				int bpp = surface->format->BytesPerPixel;
				Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
				if((x > surface->w) || (y > surface->h) || (x < 0) || (y < 0)) return 0;
				*(Uint32 *)p = pixel;*/
    }
}

void draw_test3(SDL_Surface* surface)
{

    //	static int splash = SPLASH_NONE;

    /*	int x;
		int y;
		for (y = 0; y < 100; y++)
			for (x = 0; x < 100; x++)
			{
				sdl_set_pixel(surface, x, y, 0x00ffffff);
			}*/
}

void draw_splash(SDL_Surface* surface)
{
    int i;
    int w = 240;
    int h = 320;
    int x = 0;
    int y = 0;
    int b = 0xc0;
    for (i = 0; i < 8; i++) {
        uint32_t clr = (b << 16) | (b << 8) | b;
        sdl_draw_rect(surface, x, y, w, h, clr);
        x++;
        y++;
        w -= 2;
        h -= 2;
        b -= 0x10;
    }
    sdl_fill_rect(surface, x, y, w, h, 0x00606060);
    sdl_draw_text(surface, &sdl_font_12x12, 10, 10, "Marlin_A3ides", 0x00e0a000);
    sdl_draw_text(surface, &sdl_font_12x12, 10, 30, "SDL simulator", 0x00808000);
    char text[32];
    sprintf(text, "%d %d", jogwheel_encoder, jogwheel_button_down);
    sdl_draw_text(surface, &sdl_font_12x12, 10, 50, text, 0x00c0c0c0);
    sprintf(text, "%d", (int)clock());
    sdl_draw_text(surface, &sdl_font_12x12, 10, 80, text, 0x00c0c0c0);
}

void draw_sim(SDL_Surface* surface)
{
    memcpy(surface->pixels, sim_st7789v_pixels, ST7789_COLS * ST7789_ROWS * 4);
}

void surface_draw(SDL_Surface* surface)
{
    static clock_t splash_timer = -1;
    if (splash_timer == 0)
        draw_sim(surface);
    else if (splash_timer == -1)
        splash_timer = clock();
    else if ((clock() - splash_timer) < 2000)
        draw_splash(surface);
    else
        splash_timer = 0;
}

extern void app_run(void);

pthread_t defaultThread;
void* defaultThreadFunc(void* pv)
{
    app_run();
    return 0;
}

extern void gui_run(void);

pthread_t displayThread;
void* displayThreadFunc(void* pv)
{
    gui_run();
    return 0;
}

int main(int argc, char** argv)
{
    int ret;
    int quit = 0;
    clock_t jogw_timer_enc = 0;
    clock_t jogw_timer_btn = 0;
    int32_t jogw_enc_start = 0;
    SDL_Surface* surface = 0;
    printf("%s start\n", appname);
    ret = SDL_Init(SDL_INIT_EVERYTHING);
    if (ret != 0) {
        printf("SDL_Init failed (%d) start\n", ret);
        return -1;
    }
    SDL_WM_SetCaption(appname, NULL);
    surface = SDL_SetVideoMode(ST7789_COLS, ST7789_ROWS, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);

    pthread_create(&defaultThread, 0, &defaultThreadFunc, 0);
    pthread_create(&displayThread, 0, &displayThreadFunc, 0);

    while (!quit) {
        timer_loop_old = timer_loop;
        timer_loop = clock();
        if (timer_loop_old)
            timer_loop_delta = timer_loop - timer_loop_old;
        SDL_Flip(surface);
        SDL_LockSurface(surface);
        surface_draw(surface);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_1:
                    draw_test1(surface);
                    break;
                case SDLK_2:
                    draw_test2(surface);
                    break;
                case SDLK_3:
                    draw_test3(surface);
                    break;
                case SDLK_ESCAPE:
                case SDLK_x:
                    quit = 1;
                    break;
                case SDLK_LEFT:
                    jogw_flags |= JOGW_MINUS;
                    jogwheel_encoder--;
                    jogw_enc_start = jogwheel_encoder;
                    break;
                case SDLK_RIGHT:
                    jogw_flags |= JOGW_PLUS;
                    jogwheel_encoder++;
                    jogw_enc_start = jogwheel_encoder;
                    break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                case SDLK_KP_ENTER:
                    jogw_flags |= JOGW_BUTTON;
                    jogwheel_button_down = 1;
                    break;
                default:
                    break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                    jogw_flags &= ~JOGW_MINUS;
                    jogw_timer_enc = 0;
                    break;
                case SDLK_RIGHT:
                    jogw_flags &= ~JOGW_PLUS;
                    jogw_timer_enc = 0;
                    break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                case SDLK_KP_ENTER:
                    jogw_flags &= ~JOGW_BUTTON;
                    jogwheel_button_down = 0;
                    jogw_timer_btn = 0;
                    break;
                default:
                    break;
                }
                break;
            }
        }
        SDL_UnlockSurface(surface);

        if ((jogw_flags & JOGW_MINUS) && !(jogw_flags & JOGW_PLUS)) {
            clock_t dt = timer_loop - (jogw_timer_enc + JOGW_AR_PAUSE);
            if (jogw_timer_enc == 0)
                jogw_timer_enc = timer_loop;
            else if (dt >= JOGW_AR_DELAY)
                jogwheel_encoder = jogw_enc_start - (dt / JOGW_AR_DELAY);
        }

        if ((jogw_flags & JOGW_PLUS) && !(jogw_flags & JOGW_MINUS)) {
            clock_t dt = timer_loop - (jogw_timer_enc + JOGW_AR_PAUSE);
            if (jogw_timer_enc == 0)
                jogw_timer_enc = timer_loop;
            else if (dt >= JOGW_AR_DELAY)
                jogwheel_encoder = jogw_enc_start + (dt / JOGW_AR_DELAY);
        }

        if (jogw_flags & JOGW_BUTTON) {
            if (jogw_timer_btn)
                jogwheel_button_down = (timer_loop - jogw_timer_btn);
            else
                jogw_timer_btn = timer_loop;
        }
    }
    SDL_Quit();
    printf("%s quit\n", appname);
    return 0;
}

//TODO: maybe conditional translation for MINGW
//fake fmemopen, because it not defined in mingw
FILE* fmemopen(void* buf, size_t size, const char* opentype)
{
    FILE* file = 0;
    if (strcmp(opentype, "r") == 0) {
        file = tmpfile();
        fwrite(buf, 1, size, file);
        rewind(file);
    }
    return file;
}

// windows startup
#ifdef _WIN32

#include <Windows.h>
#include <fcntl.h>

#define MAX_CMD_LINE 8192
#define MAX_CMD_ARGS 64

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    char argstr[MAX_CMD_LINE]; // buffer for commandline
    char* arg = argstr; // argument pointer for iterating
    int argc = 0; // number of arguments
    BOOL bret; // windows result variable (BOOL)
    int ret; // result variable
    char* argv[MAX_CMD_ARGS]; //argument array
    char ch; //
    // close all std handles - this is because we want have stdin=0, stdout=1 and stderr=2
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
    ret = AllocConsole(); // allocate new console window
    if (!ret)
        return -1;
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE); // obtain console input handle
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE); // obtain console output handle
    int nInput = _open_osfhandle((intptr_t)hInput, _O_RDONLY | _O_TEXT); // convert input handle to fileno
    int nOutput = _open_osfhandle((intptr_t)hOutput, _O_TEXT); // convert output handle to fileno
    int nError = dup(nOutput); // duplicate output to error
#if 0
	if ((nInput != 0) || (nOutput != 1) || (nError != 2))
		return -1; // check file numbers
#endif
    // open files
    FILE* _fi = _fdopen(nInput, "r");
    FILE* _fo = _fdopen(nOutput, "w");
    FILE* _fe = _fdopen(nError, "w");
    // copy file structures
    *stdin = *_fi;
    *stdout = *_fo;
    *stderr = *_fe;
    // make arguments
    memset(argstr, 0, sizeof(argstr));
    memset(argv, 0, sizeof(argv));
    GetModuleFileName(hInstance, argstr, MAX_CMD_LINE); // path to executable
    argv[argc++] = arg; // append path as argument 0
    arg += strlen(arg) + 1; // move pointer
    while (strlen(lpCmdLine) > 0) // cmdline not empty
    {
        argv[argc] = arg;
        ch = *(lpCmdLine++);
#if 1 // filter '\'' chars around argument
        if (ch == '\'') // argument started with '\''
        {
            while (((ch = *(lpCmdLine++)) != 0)) {
                if (ch == '\'')
                    break; // must be terminated with '\'', not ' '
                *(arg++) = ch; // add char and increment pointer
            }
        } else
#endif
            while (((ch = *(lpCmdLine++)) != 0)) {
                if (ch == ' ')
                    break; // terminated with ' '
                *(arg++) = ch; // add char and increment pointer
            }
        *(arg++) = 0;
        if (strlen(argv[argc]) > 0) // parsed argument not empty
            argc++;
        while ((ch = *lpCmdLine) == ' ') // skip spaces
            lpCmdLine++;
    }
    //TODO: cleanup, close handles
    ret = main(argc, argv); // run main
    printf("Press any key...");
    getch(); // wait for key press in console
    return ret;
}

#endif //_WIN32
