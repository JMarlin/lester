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

void point3d(SDL_Renderer* r, int x, int y, int z) {
	
	SDL_SetRenderDrawColor(r, z, z, z, 0xFF);
	SDL_RenderDrawPoint(r, x, y);
}

void swap(int *a, int* b) {
	
	int c;
	
	c = a[0];
	a[0] = b[0];
	b[0] = c;
}


void line3d(SDL_Renderer *r, int x0, int y0, int z0, int x1, int y1, int z1) {
	
	if(x0 > x1)
		swap(&x0, &x1);
	
	if(y0 > y1)
		swap(&y0, &y1);
		
	if(z0 > z1)
		swap(&z0, &z1);
		
    int dx3  = abs(x1 - x0);
    int sx3  = x0 < x1 ? 1 : -1;
    int dy3  = abs(y1 - y0);
	int dz3  = abs(z1 - z0);
    int err3 = (dx3 > dy3 ? dx3 : -dy3)/2;
    int x3 = x0;
    int y3 = y0;
	int z3 = z0;
    int endy = y1;
	int e23;
	
	 while(1) {
	    
		point3d(r, x3, y3, z3);
		        
        if(y3 == endy) {
            
            break;
        }
        
        e23 = err3;
        
        if(e23 > -dx3) {
            
            err3 -= dy3;
            x3 += sx3;
        }
		
		if(e23 > -dz3) {
            
            err3 -= dy3;
            z3 += 1;
        }
		
        if(e23 < dy3) {
            
            err3 += dx3;
            y3 += 1;
        }
    }
}

void line3d_better_but_shitty(SDL_Renderer *r, int x0, int y0, int z0, int x1, int y1, int z1) {
	
	int x, delta_x, step_x;
	int y, delta_y, step_y;
	int z, delta_z, step_z;
	int swap_xy, swap_xz;
	int drift_xy, drift_xz;
	int cx, cy, cz, stepCount;
	
	swap_xy = ABS(y1 - y0) > ABS(x1 - x0);
	
	if(swap_xy) {
		
		swap(&x0, &y0);
		swap(&x1, &y1);
	}
	
	swap_xz = ABS(z1 - z0) > ABS(x1 - x0);
	
	if(swap_xz) {
		
		swap(&x0, &z0);
		swap(&x1, &z1);
	}
	
	delta_x = ABS(x1 - x0);
	delta_y = ABS(y1 - y0);
	delta_z = ABS(z1 - z0);
	
	drift_xy = delta_x >> 1; // div 2
	drift_xz = delta_x >> 1; // div 2
	
	step_x = (x0 > x1) ? -1 : 1;
	step_y = (y0 > y1) ? -1 : 1;
	step_z = (z0 > z1) ? -1 : 1;
	
	y = y0;
	z = z0;
	x = x0;
	stepCount = 0;
	
	while(!(x == x1 && y == y1 && z == z1)) {
		
		stepCount++;
		
		cx = x;
		cy = y;
		cz = z;
		
		if(swap_xz)
			swap(&cx, &cz);
			
		if(swap_xy)
			swap(&cx, &cy);
			
		point3d(r, cx, cy, cz);
		
		drift_xy -= delta_y;
		drift_xz -= delta_z;
		
		if(drift_xy < 0) {
			
			y += step_y;
			drift_xy += delta_x;
		}	
		
		if(drift_xz < 0) {
			
			z += step_z;
			drift_xz += delta_x;
		}
		
		x += step_x;
	}
}

void line3d_old(SDL_Renderer *r, int x1, int y1, int z1, int x2, int y2, int z2) {
	
	int xd, yd, zd;
	int x, y, z;
	int ax, ay, az;
	int sx, sy, sz;
	int dx, dy, dz;
	
	dx = x2 - x1;
	dy = y2 - y1;
	dz = z2 - z1;
	
	ax = ABS(dx) << 1;
	ay = ABS(dy) << 1;
	az = ABS(dz) << 1;
	
	sx = ZSGN(dx);
	sy = ZSGN(dy);
	sz = ZSGN(dz);
	
	x = x1;
	y = y1;
	z = z1;
	
	if(ax >= MAX(ay, az)) {
		yd = ay - (ax >> 1);
		zd = az - (ax >> 1);
		
		while(1) {
			
			point3d(r, x, y, z);
			
			if(x == x2)
				return;
				
			if(yd >= 0) {
				
				y += sy;
				yd -= ax;
			}
			
			if(zd >= 0) {
				
				z += sz;
				zd -= ax;
			}
			
			x += sx;
			yd += ay;
			zd += az;
		}
	} else if(ay >= MAX(ax, az)) {
		
		xd = ax - (ay >> 1);
		zd = az - (ay >> 1);
		
		while(1) {
			
			point3d(r, x, y ,z);
			
			if(y == y2) 
				return;
				
			if(xd >= 0) {
				
				x += sx;
				xd -= ay;
			}
			
			if(zd >= 0) {
				
				z += sz;
				zd -= ay;
			}
			
			y += sy;
			xd += ax;
			zd += az;
		}
	} else if (az >= MAX(ax, ay)) {
		
		xd = ax - (az >> 1);
		yd = ay - (az >> 1);
		
		while(1) {
			
			point3d(r, x, y, z);
			
			if(z == z2)
				return;
				
			if(xd >= 0) {
				
				x += sx;
				xd -= az;
			}
			
			if(yd >= 0) {
				
				y += sy;
				yd -= az;
			}
			
			z += sz;
			xd += ax;
			yd += ay;
		}
	}
}

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
	/*
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
*/
	line3d(renderer, xa, yb, za, xb, ya, zb);
	
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
