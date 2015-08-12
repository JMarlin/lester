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
    int i, dx, dy, dz, l, m, n, x_inc, y_inc, z_inc, err_1, err_2, dx2, dy2, dz2;
    int point[3];
    
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
		
	//Draw line	
	point[0] = xa;
	point[1] = ya;
	point[2] = za;
	dx = xb - xa;
	dy = yb - ya;
	dz = zb - za;
	x_inc = (dx < 0) ? -1 : 1;
	l = abs(dx);
	y_inc = (dy < 0) ? -1 : 1;
	m = abs(dy);
	z_inc = (dz < 0) ? -1 : 1;
	n = abs(dz);
	dx2 = l << 1;
	dy2 = m << 1;
	dz2 = n << 1;
	
	if((l >= m) && (l >= n)) {
		
		err_1 = dy2 - l;
		err_2 = dz2 - l;
		for(i = 0; i < l; i++) {
		
			SDL_SetRenderDrawColor(renderer, point[2], point[2], point[2], 0xFF);
			SDL_RenderDrawPoint(renderer, point[0], point[1]);
			
			if(err_1 > 0) {
			
				point[1] += y_inc;
				err_1 -= dx2;
			}
			
			if(err_2 > 0) {
			
				point[2] += z_inc;
				err_2 -= dx2;
			}
			
			err_1 += dy2;
			err_2 += dz2;
			point[0] += x_inc;
		}
	} else if((m >= l) && (m >= n)) {
		
		err_1 = dx2 - m;
		err_2 = dz2 - m;
		for(i = 0; i < m; i++) {
		
			SDL_SetRenderDrawColor(renderer, point[2], point[2], point[2], 0xFF);
			SDL_RenderDrawPoint(renderer, point[0], point[1]);
			
			if(err_1 > 0) {
			
				point[0] += x_inc;
				err_1 -= dy2;
			}
			
			if(err_2 > 0) {
			
				point[2] += z_inc;
				err_2 -= dy2;
			}
			
			err_1 += dx2;
			err_2 += dz2;
			point[1] += y_inc;
		}		
	} else {
		
		err_1 = dy2 - n;
		err_2 = dx2 - n;
		for(i = 0; i < n; i++) {
		
			SDL_SetRenderDrawColor(renderer, point[2], point[2], point[2], 0xFF);
			SDL_RenderDrawPoint(renderer, point[0], point[1]);
			
			if(err_1 > 0) {
			
				point[1] += y_inc;
				err_1 -= dz2;
			}
			
			if(err_2 > 0) {
			
				point[0] += x_inc;
				err_2 -= dz2;
			}
			
			err_1 += dy2;
			err_2 += dx2;
			point[2] += z_inc;
		}				
	}	 
	
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
