#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_PIXELS SCREEN_WIDTH * SCREEN_HEIGHT
#define SCREEN_DEPTH 5.0

//Convert a point scaled such that 1.0, 1.0 is at the upper right-hand
//corner of the screen and -1.0, -1.0 is at the bottom right to pixel coords
#define PI 3.141592653589793
#define TO_SCREEN_Y(y) ((int)((SCREEN_HEIGHT-(y*SCREEN_HEIGHT))/2.0))
#define TO_SCREEN_X(x) ((int)((SCREEN_WIDTH+(x*SCREEN_HEIGHT))/2.0))
#define TO_SCREEN_Z(z) ((unsigned char)((z) > SCREEN_DEPTH ? 0 : (255.0 - ((z*255.0)/SCREEN_DEPTH))))
#define DEG_TO_RAD(a) ((((float)a)*PI)/180.0)
#define MAX(a,b) (((a)>(b)?(a):(b)))
#define ABS(a) (((a)<0)?-(a):(a))
#define ZSGN(a) (((a)<0)?-1:(a)>0?1:0)

int main(int argc, char* argv[]) {

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event e;
    int done = 0;
    int xa = 10;
    int ya = 10;
    int za = 0;
    int xb = 100;
    int yb = 75;
    int zb = 255;
    
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {

        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow("LESTER", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if(window == NULL) {

        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    if(renderer == NULL) {

        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

	SDL_SetRenderDrawColor(renderer, 0x20, 0x50, 0x60, 0xFF);
	SDL_RenderClear(renderer);
	triangle3d(renderer, 320, 0, 70, 0, 300, 0, 500, 400, 255);
	//line3d(renderer, xa, ya, za, xb, yb, zb);
    SDL_RenderPresent(renderer);

    while(!done) {

        while( SDL_PollEvent( &e ) != 0 ) {
        
            if( e.type == SDL_QUIT ) 
                done = 1;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
