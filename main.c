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

float focal_length;
unsigned char *zbuf;

typedef struct point {
    float x;
    float y;
} point;

typedef struct color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} color;

typedef struct vertex {
    float x;
    float y;
    float z;
    color *c;
} vertex;

typedef struct triangle {
    vertex v[3];
} triangle;

typedef struct node {
    void *payload;
    struct node *next;
} node;

typedef struct list {
    node *root;
} list;

typedef struct object {
    list tri_list;
    float x;
    float y;
    float z;
} object;

#define list_for_each(l, i, n) for((i) = (l)->root, (n) = 0; (i) != NULL; (i) = (i)->next, (n)++)
#define new(x) ((x*)malloc(sizeof(x)))

void clear_zbuf() {
    
    memset((void*)zbuf, 0, SCREEN_PIXELS);  
}

int init_zbuf() {
    
    zbuf = (unsigned char*)malloc(SCREEN_PIXELS);
    
    if(!zbuf)
        return 0;
    
    clear_zbuf();  
        
    return 1;
}

void clone_color(color* src, color* dst) {
    
    dst->r = src->r;
    dst->g = src->g;
    dst->b = src->b;
    dst->a = src->a;
}

node *list_get_last(list *target) {
    
    int i;
    node *list_node;
    
    list_for_each(target, list_node, i) {
        
        if(list_node->next == NULL)
            return list_node;
    }
    
    return NULL;
}

void list_push(list *target, void* item) {
    
    node *last     = list_get_last(target);
    node *new_node = new(node);

    if(!new_node) 
         return;
    

    new_node->payload = item;
    new_node->next = NULL;
        
    if(!last) {
     
        target->root = new_node;
    } else {      
     
        last->next = new_node;
    }
}

void dump_list(list *target) {
    
    node *item;
    
    printf("list.root = 0x%08x\n", (unsigned int)target->root);
    item = target->root;
    
    while(item) {
        
        printf("   Item 0x%08x:\n", (unsigned int)item);
        printf("      payload: 0x%08x\n", (unsigned int)item->payload);
        printf("      next: 0x%08x\n", (unsigned int)item->next);
        item = item->next;
    }
}

color *new_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    
    color *ret_color = new(color);
    
    if(!ret_color)
        return ret_color;
    
    ret_color->r = r;
    ret_color->g = g;
    ret_color->b = b;
    ret_color->a = a;
    
    return ret_color;
}

void clone_vertex(vertex *src, vertex* dst) {
    
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
    dst->c = src->c;
}

triangle *new_triangle(vertex *v1, vertex *v2, vertex *v3) {
    
    triangle *ret_tri = new(triangle);
    
    if(!ret_tri)
        return ret_tri;
        
    clone_vertex(v1, &(ret_tri->v[0]));
    clone_vertex(v2, &(ret_tri->v[1]));
    clone_vertex(v3, &(ret_tri->v[2]));    
    
    return ret_tri;
}

void purge_list(list *target) {
    
    node *temp_node, *next_node;
    
    for(temp_node = target->root; temp_node != NULL;) {
        next_node = temp_node->next;
        free(temp_node);
        temp_node = next_node;
    }
}

void delete_object(object *obj) {
    
    node *item;
    int i;
    
    list_for_each(&(obj->tri_list), item, i) {
        
        free(item->payload);
    }
    
    purge_list(&(obj->tri_list));
    free(obj);
}

object *new_object() {
    
    object *ret_obj = new(object);
    
    if(!ret_obj)
        return ret_obj;
        
    ret_obj->tri_list.root = NULL;
    ret_obj->x = ret_obj->y = ret_obj->z = 0.0;
    
    return ret_obj;
}

object *new_cube(float s, color *c) {
    
    object* ret_obj = new_object();
    
    if(!ret_obj) {
        
        printf("[new_cube] object allocation failed\n");
        return ret_obj;
    }
    
    printf("[new_cube] new object allocated\n");
    
    triangle* temp_tri;
    vertex temp_v[3];
    float half_s = s/2.0;
    float points[][3] = {
        {-half_s, half_s, -half_s},
        {half_s, half_s, -half_s},
        {half_s, -half_s, -half_s},
        {-half_s, -half_s, -half_s},
        {-half_s, half_s, half_s},
        {half_s, half_s, half_s},
        {half_s, -half_s, half_s},
        {-half_s, -half_s, half_s},
    };
    const int order[][3] = {
                  {7, 5, 4}, //3
                  {6, 5, 7}, //3
                  {3, 0, 1}, //1
                  {3, 1, 2}, //1
                  {4, 5, 0}, 
                  {1, 0, 5},
                  {6, 7, 3},
                  {3, 2, 6},
                  {6, 1, 5},
                  {6, 2, 1},
                  {7, 4, 0},
                  {0, 3, 7}
              };
    int i;
    
    if(!ret_obj)
        return ret_obj;
    
    temp_v[0].c = c;
    temp_v[1].c = c;
    temp_v[2].c = c;
    
    for(i = 0; i < 12; i++) {
     
        temp_v[0].x = points[order[i][0]][0];
        temp_v[0].y = points[order[i][0]][1];
        temp_v[0].z = points[order[i][0]][2];
        temp_v[1].x = points[order[i][1]][0];
        temp_v[1].y = points[order[i][1]][1];
        temp_v[1].z = points[order[i][1]][2];
        temp_v[2].x = points[order[i][2]][0];
        temp_v[2].y = points[order[i][2]][1];
        temp_v[2].z = points[order[i][2]][2];
        
        printf("[new_cube] Creating new triangle (%f, %f, %f), (%f, %f, %f), (%f, %f, %f)\n", temp_v[0].x, temp_v[0].y, temp_v[0].z, temp_v[1].x, temp_v[1].y, temp_v[1].z,  temp_v[2].x, temp_v[2].y, temp_v[2].z);
        
        if(!(temp_tri = new_triangle(&temp_v[0], &temp_v[1], &temp_v[2]))) {
            
            printf("[new_cube] failed to allocate triangle #%d\n", i+1);
            delete_object(ret_obj);
            return NULL;        
        }
        printf("[new_cube] generated triangle #%d\n", i+1);
        
        list_push(&(ret_obj->tri_list), (void*)temp_tri);
        printf("[new_cube] inserted triangle #%d\n", i+1);
    }
    
    return ret_obj;
}

void translate_object(object* obj, float x, float y, float z) {
    
    triangle *temp_tri;
    node     *item;
    int      i, j;
    
    obj->x += x;
    obj->y += y;
    obj->z += z;
    
    list_for_each(&(obj->tri_list), item, i) {
        
        temp_tri = (triangle*)(item->payload);
        
        for(j = 0; j < 3; j++) {
        
            temp_tri->v[j].x += x;
            temp_tri->v[j].y += y;
            temp_tri->v[j].z += z;
        }
    }
}

void rotate_object_x_global(object* obj, float angle) {
    
    float rad_angle = DEG_TO_RAD(angle);
    triangle *temp_tri;
    node     *item;
    int      i, j;
    float temp_y, temp_z;
        
    list_for_each(&(obj->tri_list), item, i) {
        
        temp_tri = (triangle*)(item->payload);
        
        for(j = 0; j < 3; j++) {
        
            temp_y = temp_tri->v[j].y;
            temp_z = temp_tri->v[j].z;
        
            temp_tri->v[j].y = (temp_y * cos(rad_angle)) - (temp_z * sin(rad_angle));
            temp_tri->v[j].z = (temp_y * sin(rad_angle)) + (temp_z * cos(rad_angle));
        }
    }
}

void rotate_object_y_global(object* obj, float angle) {
    
    float rad_angle = DEG_TO_RAD(angle);
    triangle *temp_tri;
    node     *item;
    int      i, j;
    float temp_x, temp_z;
        
    list_for_each(&(obj->tri_list), item, i) {
        
        temp_tri = (triangle*)(item->payload);
        
        for(j = 0; j < 3; j++) {
            
            temp_x = temp_tri->v[j].x;
            temp_z = temp_tri->v[j].z;
            
            temp_tri->v[j].x = (temp_x * cos(rad_angle)) + (temp_z * sin(rad_angle));
            temp_tri->v[j].z = (temp_z * cos(rad_angle)) - (temp_x * sin(rad_angle));
        }
    }
}

void rotate_object_z_global(object* obj, float angle) {
    
    float rad_angle = DEG_TO_RAD(angle);
    triangle *temp_tri;
    node     *item;
    int      i, j;
    float temp_x, temp_y;
        
    list_for_each(&(obj->tri_list), item, i) {
        
        temp_tri = (triangle*)(item->payload);
        
        for(j = 0; j < 3; j++) {
            
            temp_x = temp_tri->v[j].x;
            temp_y = temp_tri->v[j].y;
            
            temp_tri->v[j].x = (temp_x * cos(rad_angle)) - (temp_y * sin(rad_angle));
            temp_tri->v[j].y = (temp_x * sin(rad_angle)) + (temp_y * cos(rad_angle));
        }
    }
}

void rotate_object_x_local(object* obj, float angle) {
    
    float oldx, oldy, oldz;
    
    oldx = obj->x;
    oldy = obj->y;
    oldz = obj->z;
    
    translate_object(obj, -oldx, -oldy, -oldz);
    rotate_object_x_global(obj, angle);
    translate_object(obj, oldx, oldy, oldz);
}

void rotate_object_y_local(object* obj, float angle) {
    
    float oldx, oldy, oldz;
    
    oldx = obj->x;
    oldy = obj->y;
    oldz = obj->z;
    
    translate_object(obj, -oldx, -oldy, -oldz);
    rotate_object_y_global(obj, angle);
    translate_object(obj, oldx, oldy, oldz);
}

void rotate_object_z_local(object* obj, float angle) {
    
    float oldx, oldy, oldz;
    
    oldx = obj->x;
    oldy = obj->y;
    oldz = obj->z;
    
    translate_object(obj, -oldx, -oldy, -oldz);
    rotate_object_z_global(obj, angle);
    translate_object(obj, oldx, oldy, oldz);
}

void project(vertex* v, point* p) {

    float delta = (v->z == 0.0) ? 1.0 : (focal_length/v->z);

    p->x = v->x * delta;
    p->y = v->y * delta;
}

void render_triangle(triangle* tri, SDL_Renderer *rend) {
    
    int i;
    int x1, y1, dx1, dy1, sx1, err1, e21;
    int x2, y2, dx2, dy2, sx2, err2, e22;
    int x3, y3, dx3, dy3, sx3, err3, e23;
    point p[3];
    point split;
    float vec_a[3];
    float vec_b[3];
    float cross[3];
    float mag;
    float normal_angle;
    float lighting_pct;
    float r, g, b;
    unsigned char f, s, t, e, done_1, done_2;
    
    //Calculate the surface normal
    //subtract 3 from 2 and 1, translating it to the origin
    vec_a[0] = tri->v[0].x - tri->v[2].x;
    vec_a[1] = tri->v[0].y - tri->v[2].y;
    vec_a[2] = tri->v[0].z - tri->v[2].z;
    vec_b[0] = tri->v[1].x - tri->v[2].x;
    vec_b[1] = tri->v[1].y - tri->v[2].y;
    vec_b[2] = tri->v[1].z - tri->v[2].z;
    
    //calculate the cross product using 1 as vector a and 2 as vector b
    cross[0] = vec_a[1]*vec_b[2] - vec_a[2]*vec_b[1];
    cross[1] = vec_a[2]*vec_b[0] - vec_a[0]*vec_b[2];
    cross[2] = vec_a[0]*vec_b[1] - vec_a[1]*vec_b[0]; 
    
    //normalize the result vector
    mag = sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);
    cross[0] /= mag;
    cross[1] /= mag;
    cross[2] /= mag;
        
    //Calculate the normal's angle vs the camera view direction
    normal_angle = acos(-cross[2]);
    
    //If the normal is facing away from the camera, don't bother drawing it
    if(normal_angle >= (PI/2))
        return;
    
    lighting_pct = ((2.0*(PI - normal_angle)) / PI) - 1.0;
    r = tri->v[0].c->r * lighting_pct;
    r = r > 255.0 ? 255 : r;     
    g = tri->v[0].c->g * lighting_pct;
    g = g > 255.0 ? 255 : g;
    b = tri->v[0].c->b * lighting_pct;
    b = b > 255.0 ? 255 : b;
    
    //Also perform a TO_SCREEN_Z of the z-value to be used in z-buffer calculations
    for(i = 0; i < 3; i++) {
        
        project(&(tri->v[i]), &p[i]);
        //printf("%f", p[i].y);
        p[i].x = TO_SCREEN_X(p[i].x);
        p[i].y = TO_SCREEN_Y(p[i].y);
        //printf("(%f) ", p[i].y);
    }
    //printf("\n");
    
    //sort vertices by ascending y
    f = 0; s = 1; t = 2;
    if(p[f].y > p[s].y) {
        e = s;
        s = f;
        f = e;
    }
    if(p[s].y > p[t].y) {
        e = t;
        t = s;
        s = e;
    }
    if(p[f].y > p[s].y) {
        e = s;
        s = f;
        f = e;
    }
    
    //printf("Ordered ys: %f, %f, %f\n", p[f].y, p[s].y, p[t].y);
    
    //Set up bresenham starting values
    dx1  = abs((int)p[s].x - (int)p[f].x);
    sx1  = (int)p[f].x < (int)p[s].x ? 1 : -1;
    dy1  = abs((int)p[s].y - (int)p[f].y);
    err1 = (dx1 > dy1 ? dx1 : -dy1)/2;
    dx2  = abs((int)p[t].x - (int)p[s].x);
    sx2  = (int)p[s].x < (int)p[t].x ? 1 : -1;
    dy2  = abs((int)p[t].y - (int)p[s].y);
    err2 = (dx2 > dy2 ? dx2 : -dy2)/2;
    dx3  = abs((int)p[t].x - (int)p[f].x);
    sx3  = (int)p[f].x < (int)p[t].x ? 1 : -1;
    dy3  = abs((int)p[t].y - (int)p[f].y);
    err3 = (dx3 > dy3 ? dx3 : -dy3)/2;
    done_1 = 0;
    done_2 = 0;
    x1 = (int)p[f].x;
    y1 = (int)p[f].y;
    x2 = (int)p[s].x;
    y2 = (int)p[s].y;
    x3 = (int)p[f].x;
    y3 = (int)p[f].y;
        
    SDL_SetRenderDrawColor(rend, (unsigned char)r, (unsigned char)g, (unsigned char)b, 0xFF);
    
    //Make this also interpolate the z-depth of the edges between the vertices, as calculated by TO_SCREEN_Z
    while(!done_2) {
           
        //printf("outer\n");   
                
        //Here, we do the loop through the line from topmost to third
        //stopping if we hit an increase in our y-value
        while(1) {
            
            if(y3 == (int)p[t].y) {
                
                done_2 = 1;
                break;
            }
            
            e23 = err3;
            
            if(e23 > -dx3) {
                
                err3 -= dy3;
                x3 += sx3;
            }
            
            if(e23 < dy3) {
                
                err3 += dx3;
                y3 += 1;
                break;
            }
        }
        
        //If we haven't finished first-to-second
        if(!done_1 || done_2) {
                
            //printf("in 1 %d, %d\n", y1, y2);
                
            //Drawing from first topmost to second until we hit a 
            //spot where our y-value changes
            while(1) {
                
                if(y1 == y2 || done_2) {
                    
                    done_1 = 1;
                    break;
                }
                
                e21 = err1;
                
                if(e21 > -dx1) {
                    
                    err1 -= dy1;
                    x1 += sx1;
                }
                
                if(e21 < dy1) {
                    
                    err1 += dx1;
                    y1 += 1;
                    //Replace this with a function which draws a horizontal line while interpolating between two z-buffer values
                    SDL_RenderDrawLine(rend, x1, y1, x3, y3);
                    break;
                }
            }
        }
        
        //Doing this instead of an else makes sure
        //we fall through as soon as we finish the 
        //first line
        if(done_1 || done_2) {
                   
            //printf("in 2 %d, %f\n", y2, p[t].y);       
                        
            while(1) {
                
                if(y2 == (int)p[t].y) {
                    
                    done_2 = 1;
                    break;
                }
                
                e22 = err2;
                    
                if(e22 > -dx2) {
                    
                    err2 -= dy2;
                    x2 += sx2;
                }
                
                if(e22 < dy2) {
                    
                    err2 += dx2;
                    y2 += 1;
                    //Replace this with a function which draws a horizontal line while interpolating between two z-buffer values
                    SDL_RenderDrawLine(rend, x2, y2, x3, y3);
                    break;
                }
            }
        }
    }
}

void render_object(object *obj, SDL_Renderer *r) {
    
    node* item;
    int i;
    
    list_for_each(&(obj->tri_list), item, i) {
     
        render_triangle((triangle*)item->payload, r);
    }
}

int main(int argc, char* argv[]) {

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event e;
    int fov_angle;
    float i = 0, step = 0.01;
    color *c;
    object *cube;
    int done = 0;

    if(!init_zbuf()) {
        
        printf("Could not init the z-buffer\n");
        return -1;
    }

    if(!(c = new_color(50, 200, 255, 255))) {
        
        printf("Could not allocate a new color\n");
        return -1;
    } 
    
    printf("Color created successfully\n");
    
    if(!(cube = new_cube(1.0, c))) {
        
        printf("Could not allocate a new cube\n");
        return -1;
    }

    printf("Cube created successfully\n");

    fov_angle = 100;
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

    translate_object(cube, 0.0, 0.0, 1.0);

    while(!done) {

        while( SDL_PollEvent( &e ) != 0 ) {
        
            if( e.type == SDL_QUIT ) 
                done = 1;
        }

        i += step;
        translate_object(cube, 0.0, 0.0, step);
        rotate_object_y_local(cube, 1);
        rotate_object_x_local(cube, 1);
        rotate_object_z_local(cube, 1);

        SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0xFF);
        SDL_RenderClear(renderer);
        
        render_object(cube, renderer);     
        
        if(i >= 1.0 && step > 0)
            step = -0.01;

        if(i <= 0.0 && step < 0)
            step = 0.01;

        SDL_RenderPresent(renderer);
        SDL_Delay(20);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
