#include <SDL.h>
#include <stdio.h>
#include <math.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

//Convert a point scaled such that 1.0, 1.0 is at the upper right-hand
//corner of the screen and -1.0, -1.0 is at the bottom right to pixel coords
#define TO_SCREEN_Y(y) ((int)((SCREEN_HEIGHT-(y*SCREEN_HEIGHT))/2.0))
#define TO_SCREEN_X(x) ((int)((SCREEN_WIDTH+(x*SCREEN_HEIGHT))/2.0))
#define DEG_TO_RAD(a) ((((float)a)*3.141592653589793)/180.0)

float focal_length;

typedef struct point {
    float x;
    float y;
} point;

typedef struct vertex {
    float x;
    float y;
    float z;
} vertex;

void project(vertex* v, point* p) {

    float delta = (v->z == 0.0) ? 1.0 : (focal_length/v->z);

    p->x = v->x * delta;
    p->y = v->y * delta;
}

int main(int argc, char* argv[]) {

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    int fov_angle;
    float step = 0.01;
    point points[4];
    vertex vertexes[4];

    printf("FOV angle:");
    scanf("%d", &fov_angle);
    printf("\n%d radians", DEG_TO_RAD(fov_angle));

    focal_length = 1.0 / (2.0 * tan(DEG_TO_RAD(fov_angle)/2.0));

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

    vertexes[0].x = -1.0;
    vertexes[0].y = 1.0;
    vertexes[1].x = 1.0;
    vertexes[1].y = 1.0;
    vertexes[2].x = 1.0;
    vertexes[2].y = -1.0;
    vertexes[3].x = -1.0;
    vertexes[3].y = -1.0;
    vertexes[0].z = vertexes[1].z = vertexes[2].z = vertexes[3].z = 1.0;

    while(1) {

        vertexes[0].z += step;
        vertexes[1].z = vertexes[2].z = vertexes[3].z = vertexes[0].z;

        project(&vertexes[0], &points[0]);
        project(&vertexes[1], &points[1]);
        project(&vertexes[2], &points[2]);
        project(&vertexes[3], &points[3]);

        SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0xFF);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0xFF, 0x0, 0x0, 0xFF);
        SDL_RenderDrawLine(renderer, TO_SCREEN_X(points[0].x), TO_SCREEN_Y(points[0].y), TO_SCREEN_X(points[1].x), TO_SCREEN_Y(points[1].y));
        SDL_RenderDrawLine(renderer, TO_SCREEN_X(points[1].x), TO_SCREEN_Y(points[1].y), TO_SCREEN_X(points[2].x), TO_SCREEN_Y(points[2].y));
        SDL_RenderDrawLine(renderer, TO_SCREEN_X(points[2].x), TO_SCREEN_Y(points[2].y), TO_SCREEN_X(points[3].x), TO_SCREEN_Y(points[3].y));
        SDL_RenderDrawLine(renderer, TO_SCREEN_X(points[3].x), TO_SCREEN_Y(points[3].y), TO_SCREEN_X(points[0].x), TO_SCREEN_Y(points[0].y));

        if(vertexes[0].z >= 2.0 && step > 0)
            step = -0.01;

        if(vertexes[0].z <= 1.0 && step < 0)
            step = 0.01;

        SDL_RenderPresent(renderer);
    }

    SDL_Delay(5000);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
