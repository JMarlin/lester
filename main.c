#include "SDL.h"
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
#define TO_SCREEN_Z(z) ((unsigned short)((z) > SCREEN_DEPTH || z < 0 ? 255 : ((z*255.0)/SCREEN_DEPTH)))
#define DEG_TO_RAD(a) ((((float)a)*PI)/180.0)

float focal_length;
unsigned char *zbuf;

typedef struct point {
    float x;
    float y;
} point;

typedef struct screen_point {
    int x;
    int y;
    unsigned char z;
} screen_point;

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
    
    memset((void*)zbuf, 255, SCREEN_PIXELS);  
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
    int i;
        triangle *temp_tri;
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
    
    if(!ret_obj) {
        
        printf("[new_cube] object allocation failed\n");
        return ret_obj;
    }
    
    printf("[new_cube] new object allocated\n");
        
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

void project(vertex* v, screen_point* p) {

    float delta = (v->z == 0.0) ? 1.0 : (focal_length/v->z);

    p->x = TO_SCREEN_X(v->x * delta);
    p->y = TO_SCREEN_Y(v->y * delta);
    p->z = TO_SCREEN_Z(v->z);
}

//Draw an rgb-colored line along the scanline from x=x1 to x=x2, interpolating
//z-values and only drawing the pixel if the interpolated z-value is less than
//the value already written to the z-buffer
void draw_scanline(SDL_Renderer *r, int scanline, int x0, int z0, int x1, int z1) {

    int dx, sx, dz, sz, err, te, z_addr;	
	   
    //don't draw off the screen
    if(scanline >= SCREEN_HEIGHT || scanline < 0)
    	return;
	    
    dx = abs(x1 - x0);
    sx = x0 < x1 ? 1 : -1;
    dz = abs(z1 - z0);
    sz = z0 < z1 ? 1 : -1;
    err = (dx > dz ? dx : dz) / 2;
	z_addr = scanline * SCREEN_WIDTH + x0;
       
    while(1) {
        
        
	if(x0 < SCREEN_WIDTH && x0 >= 0) {
        
	    //Check the z buffer and draw the point	
	    if(z0 < zbuf[z_addr]) {
            
                //Uncomment the below to view the depth buffer
                //SDL_SetRenderDrawColor(r, z0 >> 8, z0 >> 8, z0 >> 8, 0xFF);
                SDL_RenderDrawPoint(r, x0, scanline);
                zbuf[z_addr] = (unsigned short)z0;
           }
	}
        
        //Do the next step of z-interpolation along x
		if(x0 == x1)
            return;
            
		te = err;
        
		if(te > -dx) {
            
            err -= dz;
            x0 += sx;
            z_addr +=sx;
        } 
        
		if(te < dz) {
            
            err += dx;
            z0 += sz; 
        }
    }
    
    printf("done\n");
}

void draw_triangle(SDL_Renderer *rend, triangle* tri) {
    
    int i;
    screen_point p[3];
    float vec_a[3];
    float vec_b[3];
    float cross[3];
    float mag;
    float normal_angle;
    float lighting_pct;
    float r, g, b;
    unsigned char f, s, t, e;
    int dx_x1, sx_x1, dy_x1, sy_x1,	err_x1,	te_x1;
	int dx_z1, sx_z1, dy_z1, sy_z1,	err_z1,	te_z1;
	int dx_x2, sx_x2, dy_x2, sy_x2,	err_x2,	te_x2;
	int dx_z2, sx_z2, dy_z2, sy_z2,	err_z2,	te_z2;
	int dx_x3, sx_x3, dy_x3, sy_x3,	err_x3,	te_x3;
	int dx_z3, sx_z3, dy_z3, sy_z3,	err_z3,	te_z3;
	int current_s;
	int cur_x1, cur_x2, cur_x3;
	int cur_z1, cur_z2, cur_z3;
    int x1y, x2y, x3y, z1y, z2y, z3y;
    
    //Don't draw the triangle if it's offscreen
    if(tri->v[0].z < 0 && tri->v[1].z < 0 && tri->v[2].z < 0)
        return;
    
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
    //if(normal_angle >= (PI/2)) {
    //    
    //    return;
    //}
    
    //NOTE: We need to do some relatively easy math and clip all triangles to the front and rear planes
    //should basically be testing each vertex's z component to see if it's less than zero or greater than 
    //the screen depth and, if so, calculate the intersections of its connecting edges with the respective
    //planes and create two new vertices at the intersection points. We should then proceed to draw the
    //two new resultant triangles. 
    //If two of the three are beyond the same plane, we can replace each with the intersection of their
    //non-mutual edge and draw a single triangle.
    //Should be able to do this by performing intersection check on each plane in turn?
    //We can make this recursive:
    //begin split_tri (one, two, three)
    //    count = 0
    //    check one with back (if out, count++ and mark out) 
    //    check two with back (if out, count++ and mark out)
    //    check three with back (if out, count++ and mark out)
    //    switch count
    //        1: get new back intersections a and b
    //           split_tri(a, two, three) //actually make sure we build the right tri
    //           split_tri(b, two, three) //same note
    //           return
    //        2: get new back intersections a and b //different from above for the aforementioned reasons
    //           split_tri(a, b, three) //same note as always
    //           return
    //        0: //do nothing and flow through
    //    endswitch
    //    //Could probably shorten this by making two passes at the above while swapping planes
    //    count = 0
    //    check one with front (if out, count++ and mark out) 
    //    check two with front (if out, count++ and mark out)
    //    check three with front (if out, count++ and mark out)
    //    switch count
    //        1: get new front intersections a and b
    //           split_tri(a, two, three) //actually make sure we build the right tri
    //           split_tri(b, two, three) //same note
    //           return
    //        2: get new front intersections a and b //different from above for the aforementioned reasons
    //           split_tri(a, b, three) //same note as always
    //           return
    //        0: //do nothing and flow through
    //    endswitch
    //    //There are no triangles out, so we can pass this one along
    //    draw_triangle(one, two, three)
    //end    
    //
    //Calculate the shading color based on the first vertex color and the
    //angle between the camera and the surface normal
    lighting_pct = ((2.0*(PI - normal_angle)) / PI) - 1.0;
    r = tri->v[0].c->r * lighting_pct;
    r = r > 255.0 ? 255 : r;     
    g = tri->v[0].c->g * lighting_pct;
    g = g > 255.0 ? 255 : g;
    b = tri->v[0].c->b * lighting_pct;
    b = b > 255.0 ? 255 : b;
    SDL_SetRenderDrawColor(rend, (unsigned char)r, (unsigned char)g, (unsigned char)b, 0xFF);
    
    //Move the vertices from world space to screen space
    for(i = 0; i < 3; i++) 
        project(&(tri->v[i]), &p[i]);
    
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
                    
    //Now that they're sorted, calculate the bresenham values for each line
	//For the x of the first line ([x1, y1, z1] -> [x2, y2, z2])
	dx_x1 = abs(p[s].x - p[f].x);
	sx_x1 = p[f].x < p[s].x ? 1 : -1;
	dy_x1 = abs(p[s].y - p[f].y);
	sy_x1 = p[f].y < p[s].y ? 1 : -1; 
	err_x1 = (dx_x1 > dy_x1 ? dx_x1 : -dy_x1) / 2;
	te_x1;
	
	//For the z of the first line ([x1, y1, z1] -> [x2, y2, z2])
	dx_z1 = abs(p[s].z - p[f].z);
	sx_z1 = p[f].z < p[s].z ? 1 : -1;
	dy_z1 = abs(p[s].y - p[f].y);
	sy_z1 = p[f].y < p[s].y ? 1 : -1; 
	err_z1 = (dx_z1 > dy_z1 ? dx_z1 : -dy_z1) / 2;
	te_z1;
	
	//For the x of the second line ([x2, y2, z2] -> [x3, y3, z3])
	dx_x2 = abs(p[t].x - p[s].x);
	sx_x2 = p[s].x < p[t].x ? 1 : -1;
	dy_x2 = abs(p[t].y - p[s].y);
	sy_x2 = p[s].y < p[t].y ? 1 : -1; 
	err_x2 = (dx_x2 > dy_x2 ? dx_x2 : -dy_x2) / 2;
	te_x2;
	
	//For the z of the second line ([x2, y2, z2] -> [x3, y3, z3])
	dx_z2 = abs(p[t].z - p[s].z);
	sx_z2 = p[s].z < p[t].z ? 1 : -1;
	dy_z2 = abs(p[t].y - p[s].y);
	sy_z2 = p[s].y < p[t].y ? 1 : -1; 
	err_z2 = (dx_z2 > dy_z2 ? dx_z2 : -dy_z2) / 2;
	te_z2;
	
	//For the x of the third line ([x1, y1, z1] -> [x3, y3, z3])
	dx_x3 = abs(p[t].x - p[f].x);
	sx_x3 = p[f].x < p[t].x ? 1 : -1;
	dy_x3 = abs(p[t].y - p[f].y);
	sy_x3 = p[f].y < p[t].y ? 1 : -1; 
	err_x3 = (dx_x3 > dy_x3 ? dx_x3 : -dy_x3) / 2;
	te_x3;
	
	//For the z of the third line ([x1, y1, z1] -> [x3, y3, z3])
	dx_z3 = abs(p[t].z - p[f].z);
	sx_z3 = p[f].z < p[t].z ? 1 : -1;
	dy_z3 = abs(p[t].y - p[f].y);
	sy_z3 = p[f].y < p[t].y ? 1 : -1; 
	err_z3 = (dx_z3 > dy_z3 ? dx_z3 : -dy_z3) / 2;
	te_z3;
	
	//Set the important scanlines
	current_s = p[f].y;
	cur_x1 = p[f].x;
	cur_z1 = p[f].z;
	cur_x2 = p[s].x;
	cur_z2 = p[s].z;;
	cur_x3 = p[f].x;
	cur_z3 = p[f].z;
	
	while(current_s < p[t].y) {
		
		//Run either line 1 or line 2
		if(current_s < p[s].y) {
			
			//Draw the current scanline from line 1 to line 3
			draw_scanline(rend, current_s, cur_x1, cur_z1, cur_x3, cur_z3);
			
			//Run the x of line 1 until we've stepped y
			while(1) {
				
				te_x1 = err_x1;
				
				if (te_x1 > -dx_x1) {
					
					//x1-increase
					err_x1 -= dy_x1;
					cur_x1 += sx_x1;
				}
				
				if (te_x1 < dy_x1) { 
					
					//s-increase
					err_x1 += dx_x1;
					break;
				}
			}
			
			//Run the z of line 1 until we've stepped y
			while(1) {
				
				te_z1 = err_z1;
				
				if (te_z1 > -dx_z1) {
					
					//z1-increase
					err_z1 -= dy_z1;
					cur_z1 += sx_z1;
				}
				
				if (te_z1 < dy_z1) { 
					
					//s-increase
					err_z1 += dx_z1;
					break;
				}
			}
		} else {
			
			//Draw the current scanline from line 2 to line 3 
			draw_scanline(rend, current_s, cur_x2, cur_z2, cur_x3, cur_z3);
			
			//Run the x of line 2 until we've stepped y
			while(1) {
				
				te_x2 = err_x2;
				                
				if (te_x2 > -dx_x2) {
					
					//x2-increase
					err_x2 -= dy_x2;
					cur_x2 += sx_x2;
				}
				
				if (te_x2 < dy_x2) { 
					
					//s-increase
					err_x2 += dx_x2;
					break;
				}
			}
			
			//Run the z of line 2 until we've stepped y
			while(1) {
				
				te_z2 = err_z2;
                				
				if (te_z2 > -dx_z2) {
					
					//z2-increase
					err_z2 -= dy_z2;
					cur_z2 += sx_z2;
				}
				
				if (te_z2 < dy_z2) { 
					
					//s-increase
					err_z2 += dx_z2;
					break;
				}
			}
		}

		//Run line 3		
		//Run the x of line 3 until we've stepped y
		while(1) {
			
			te_x3 = err_x3;
			
			if (te_x3 > -dx_x3) {
				
				//x3-increase
				err_x3 -= dy_x3;
				cur_x3 += sx_x3;
			}
			
			if (te_x3 < dy_x3) { 
				
				//s-increase
				err_x3 += dx_x3;
				break;
			}
		}
		
		//Run the z of line 3 until we've stepped y
		while(1) {
			
			te_z3 = err_z3;
			
			if (te_z3 > -dx_z3) {
				
				//z3-increase
				err_z3 -= dy_z3;
				cur_z3 += sx_z3;
			}
			
			if (te_z3 < dy_z3) { 
				
				//s-increase
				err_z3 += dx_z3;
				break;
			}
		}
		        
		//Move to the next scanline		
		current_s++;
	}
}

void clip_and_render(SDL_Renderer *r, triangle* tri) {    

    int count;
    int on_second_iteration = 0;
    int i;
    float plane_z = 1;
    float scale_factor, dx, dy, dz, ndz;
    unsigned char point_marked[3] = {0, 0, 0};
    vertex new_point[2]; 
    triangle out_triangle[2];
    int fixed[2];
    int original;
    
    
    //Note that in the future we're also going to need to clip on the
    //'U', 'V' and color axes
    while(1) {
        
        count = 0;
        
        //Check each point to see if it's greater than the plane
        for(i = 0; i < 3; i++) {
            
            if((on_second_iteration && tri->v[i].z > plane_z) || 
              (!on_second_iteration && tri->v[i].z < plane_z)) {

                point_marked[i] = 1;
                count++;
            } else {
                
                point_marked[i] = 0;
            }
        }
            
        switch(count) {
            
            //If all of the vertices were out of range, 
            //skip drawing the whole thing entirely
            case 3:
                return;
                break;
            
            //If one vertex was out, find it's edge intersections and
            //build two new triangles out of it
            case 1:
                //Figure out what the other two points are
                fixed[0] = point_marked[0] ? point_marked[1] ? 2 : 1 : 0;
                fixed[1] = fixed[0] == 0 ? point_marked[1] ? 2 : 1 : fixed[0] == 1 ? point_marked[0] ? 2 : 0 : point_marked[0] ? 1 : 0;
                original = point_marked[0] ? 0 : point_marked[1] ? 1 : 2;
                
                //Calculate the new intersection points
                for(i = 0; i < 2; i++) {
                    
                    //x,y, and z 'length'
                    dx = tri->v[original].x - tri->v[fixed[i]].x;
                    dy = tri->v[original].y - tri->v[fixed[i]].y;
                    dz = tri->v[original].z - tri->v[fixed[i]].z;
                                       
                    //Set the known axis value
                    new_point[i].z = plane_z; //Replace this with a line function
                    
                    //z 'length' of new point
                    ndz = new_point[i].z - tri->v[fixed[i]].z;
                    
                    //ratio of new y-length to to old
                    scale_factor = ndz/dz; //For now, we're dealing with a plane orthogonal to the clipping axis and as such 
                                           //we can't possibly have zero dy because that would place both the 'in' and 'out'
                                           //vertexes behind the plane, which is obviously impossible, so we won't worry about
                                           //that case until we start playing with sloped clipping planes
                    
                    //Scale the independent axis value by the scaling factor
                    //We can do this for other arbitrary axes in the future, such as U and V
                    new_point[i].x = scale_factor * dx + tri->v[fixed[i]].x;
                    new_point[i].y = scale_factor * dy + tri->v[fixed[i]].y;
                    
                    //Copy the color information
                    new_point[i].c = tri->v[fixed[i]].c;
                }   
                
                
                //Test/draw the new triangles, maintaining the CW or CCW ordering
                //Same starting points for all triangles
                clone_vertex(&new_point[0], &(out_triangle[0].v[0]));
                clone_vertex(&new_point[1], &(out_triangle[1].v[0]));
                
                //do corrections for CW/CCW
                if(fixed[1] < fixed[0]) {
                    
                    //Build the first triangle
                    clone_vertex(&(tri->v[fixed[0]]), &(out_triangle[0].v[1]));
                    clone_vertex(&(tri->v[fixed[1]]), &(out_triangle[0].v[2]));
                    
                    //Build the second triangle
                    clone_vertex(&new_point[0], &(out_triangle[1].v[1]));
                    clone_vertex(&(tri->v[fixed[1]]), &(out_triangle[1].v[2]));
                } else {
                    
                    //Build the first triangle
                    clone_vertex(&(tri->v[fixed[1]]), &(out_triangle[0].v[1]));
                    clone_vertex(&(tri->v[fixed[0]]), &(out_triangle[0].v[2]));
                    
                    //Build the second triangle
                    clone_vertex(&(tri->v[fixed[1]]), &(out_triangle[1].v[1]));
                    clone_vertex(&new_point[0], &(out_triangle[1].v[2]));
                }
                
                //Run the new triangles through another round of processing
                clip_and_render(r, &out_triangle[0]);
                clip_and_render(r, &out_triangle[1]);
                
                //Exit the function early for dat tail recursion              
                return;
                break;
            
            case 2:
                //Figure out which point we're keeping
                original = point_marked[0] ? point_marked[1] ? 2 : 1 : 0;
                fixed[0] = point_marked[0] ? 0 : point_marked[1] ? 1 : 2;
                fixed[1] = fixed[0] == 0 ? point_marked[1] ? 1 : 2 : fixed[0] == 1 ? point_marked[0] ? 0 : 2 : point_marked[0] ? 0 : 1;
                            
                //Calculate the new intersection points
                for(i = 0; i < 2; i++) {
                    
                    //x,y, and z 'length'
                    dx = tri->v[original].x - tri->v[fixed[i]].x;
                    dy = tri->v[original].y - tri->v[fixed[i]].y;
                    dz = tri->v[original].z - tri->v[fixed[i]].z;
                                       
                    //Set the known axis value
                    new_point[i].z = plane_z; //Replace this with a line function
                    
                    //z 'length' of new point
                    ndz = new_point[i].z - tri->v[fixed[i]].z;
                    
                    //ratio of new y-length to to old
                    scale_factor = ndz/dz; //For now, we're dealing with a plane orthogonal to the clipping axis and as such 
                                           //we can't possibly have zero dy because that would place both the 'in' and 'out'
                                           //vertexes behind the plane, which is obviously impossible, so we won't worry about
                                           //that case until we start playing with sloped clipping planes
                    
                    //Scale the independent axis value by the scaling factor
                    //We can do this for other arbitrary axes in the future, such as U and V
                    new_point[i].x = scale_factor * dx + tri->v[fixed[i]].x;
                    new_point[i].y = scale_factor * dy + tri->v[fixed[i]].y;
                    
                    //Copy the color information
                    new_point[i].c = tri->v[fixed[i]].c;
                }      
                
                //Start building the new triangles, maintaining the CW or CCW ordering
                if(fixed[0] < fixed[1]) {      
                                  
                    clone_vertex(&new_point[0], &(out_triangle[0].v[0]));
                    clone_vertex(&new_point[1], &(out_triangle[0].v[1]));
                } else {
                    
                    //render_clipped_triangles(r, new_point[1], new_point[0], tri->v[original]);
                    clone_vertex(&new_point[1], &(out_triangle[0].v[0]));
                    clone_vertex(&new_point[0], &(out_triangle[0].v[1]));
                }
                
                //Always ends on the same point
                clone_vertex(&(tri->v[original]), &(out_triangle[0].v[2]));
                
                //Send through processing again
                clip_and_render(r, &out_triangle[0]);
                    
                //Exit the function early for dat tail recursion  
                return;
                break;
            
            //If there were no intersections we won't do anything and 
            //allow execution to flow through
            case 0:
            default:
                break; 
        }

        //If we hit case 0 both times above, all points on this
        //triangle lie in the drawable area and we can leave this
        //clipping loop and flow down to do the drawing of the 
        //processed triangle         
        if(on_second_iteration)
            break;
        
        on_second_iteration = 1;
        plane_z = SCREEN_DEPTH;
    }    
    
    //If we got this far, the triangle is drawable. So we should do that. Or whatever.
    draw_triangle(r, tri);   
}

void render_triangle(SDL_Renderer *rend, triangle* tri) {

    clip_and_render(rend, tri);
}

void render_object(SDL_Renderer *r, object *obj) {
    
    node* item;
    int i;
    
    list_for_each(&(obj->tri_list), item, i) {
        
        render_triangle(r, (triangle*)item->payload);
    }
}

int main(int argc, char* argv[]) {

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event e;
    int fov_angle;
    float i = 0.0, step = 0.01;
    color *c;
    object *cube;
    triangle test_tri[2];
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

    fov_angle = 90;
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
    //rotate_object_y_local(cube, 45);
    //rotate_object_x_local(cube, 45);
    //rotate_object_z_local(cube, 45);
    
    /*
    test_tri[0].v[0].x = 0.5;
    test_tri[0].v[0].y = 0.5;
    test_tri[0].v[0].z = 1.0;
    test_tri[0].v[0].c = c;
    test_tri[0].v[1].x = 0.5;
    test_tri[0].v[1].y = -0.5;
    test_tri[0].v[1].z = 1.0;
    test_tri[0].v[1].c = c;
    test_tri[0].v[2].x = -0.5;
    test_tri[0].v[2].y = -0.5;
    test_tri[0].v[2].z = 1.0;
    test_tri[0].v[2].c = c;
    test_tri[1].v[0].x = 0.5;
    test_tri[1].v[0].y = 0.5;
    test_tri[1].v[0].z = 1.0;
    test_tri[1].v[0].c = c;
    test_tri[1].v[1].x = -0.5;
    test_tri[1].v[1].y = -0.5;
    test_tri[1].v[1].z = 1.0;
    test_tri[1].v[1].c = c;
    test_tri[1].v[2].x = -0.5;
    test_tri[1].v[2].y = 0.5;
    test_tri[1].v[2].z = 1.0;
    test_tri[1].v[2].c = c;
    */

    while(!done) {

        while( SDL_PollEvent( &e ) != 0 ) {
        
            if( e.type == SDL_QUIT ) 
                done = 1;
        }

        i += step;
        //translate_object(cube, 0.0, 0.0, step);
        rotate_object_y_local(cube, 1);
        rotate_object_x_local(cube, 1);
        rotate_object_z_local(cube, 1);

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);
        clear_zbuf();
        
        render_object(renderer, cube);  
        //render_triangle(renderer, &test_tri[0]);
        //render_triangle(renderer, &test_tri[1]);
        
        if(i >= 1.0 && step > 0)
            step = -0.01;

        if(i <= 0.0 && step < 0)
            step = 0.01;

        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
