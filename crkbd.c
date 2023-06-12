#include "quantum.h"
#include QMK_KEYBOARD_H
#include <stdio.h>
#include <math.h>

#ifdef SWAP_HANDS_ENABLE
__attribute__((weak)) const keypos_t PROGMEM hand_swap_config[MATRIX_ROWS][MATRIX_COLS] = {
  // Left
  {{0, 4}, {1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}},
  {{0, 5}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {5, 5}},
  {{0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}},
  {{0, 7}, {1, 7}, {2, 7}, {3, 7}, {4, 7}, {5, 7}},
  // Right
  {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}},
  {{0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}},
  {{0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2}, {5, 2}},
  {{0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}, {5, 3}}
};
#endif

// ---------
// OLED CODE
// ---------

#ifdef OLED_ENABLE

#define PI 3.14159
#define WIDTH 128
#define HEIGHT 32

#define CREATE_VEC3_OPERATION_FUNCTION(FUNNAME, OP) \
  static void FUNNAME(vec3d *res, vec3d *x, vec3d *y) { \
    res->x = x->x OP y->x; \
    res->y = x->y OP y->y; \
    res->z = x->z OP y->z; \
  }

typedef struct vec2d {
  double x;
  double y;
} vec2d;

typedef struct vec3d {
  double x;
  double y;
  double z;
} vec3d;

typedef vec3d triangle3d[3];

typedef vec2d triangle2d[3];

typedef struct mesh3d {
  triangle3d *tris;
  int tris_count;
} mesh3d;

typedef double mat4x4[4][4];

static const double obj_points[12][9] = {
    // SOUTH
		{ 0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },

		// EAST                                                      
		{ 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f },

		// NORTH                                                     
		{ 1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f },

		// WEST                                                      
		{ 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f },

		// TOP                                                       
		{ 0.0f, 1.0f, 0.0f,    0.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 0.0f },

		// BOTTOM                                                    
		{ 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f },
};


CREATE_VEC3_OPERATION_FUNCTION(sub_vec, -)

static void vec3d_to_vec2d(vec2d *res, vec3d *point) {
  res->x = point->x;
  res->y = point->y;
}

static void triangle_to_2d(triangle2d *res, triangle3d *tri) {
  for (int point_i = 0; point_i < 3; point_i++) {
    vec3d_to_vec2d(&((*res)[point_i]), &((*tri)[point_i]));
  }
}

static void load_mesh_from_obj(mesh3d *res) {
  int tris_count = sizeof(obj_points) / sizeof(obj_points[0]);

  res->tris_count = tris_count;
  res->tris = malloc(sizeof(triangle3d) * tris_count);

  for (int tri_index = 0; tri_index < tris_count; tri_index++) {
    for (int point_i = 0; point_i < 3; point_i++) {
      res->tris[tri_index][point_i].x = obj_points[tri_index][3 * point_i + 0];
      res->tris[tri_index][point_i].y = obj_points[tri_index][3 * point_i + 1];
      res->tris[tri_index][point_i].z = obj_points[tri_index][3 * point_i + 2];
    }
  }
}

static void oled_write_line(vec2d *start_p, vec2d *end_p, bool on) {
  int sx = (int)start_p->x;
  int sy = (int)start_p->y;
  int ex = (int)end_p->x;
  int ey = (int)end_p->y;
  int x_sub = ex - sx;
  int y_sub = ey - sy;
  int x_dist = fabs(x_sub);
  int y_dist = fabs(y_sub);
  double ratio = (double)y_sub / (double)x_sub;

  if (x_dist > y_dist) {
    for (int dx = 0; 1; dx += (int)(x_sub / x_dist)) {
      oled_write_pixel((uint8_t)(sx + dx), (uint8_t)(sy + ratio * dx), on);
      
      if (dx == x_sub) {
        break;
      }
    }
  } else {
    for (int dy = 0; 1; dy += (int)(y_sub / y_dist)) {
      oled_write_pixel((uint8_t)(sx + (1.0 / ratio) * dy), (uint8_t)(sy + dy), on);

      if (dy == y_sub) {
        break;
      }
    }
  }
}

static void create_projection_m(mat4x4 *res, double f_near, double f_far, double f_fov) {
  double f_aspect_ratio = (double)HEIGHT / (double)WIDTH;
  double f_fov_rad = 1.0 / tan(f_fov * 0.5 / 180.0 * PI);

  for (int i = 0; i < 4; i++) {
    memset((*res)[i], 0, 4 * sizeof(double));
  }

  (*res)[0][0] = f_fov_rad * f_aspect_ratio;
  (*res)[1][1] = f_fov_rad;
  (*res)[2][2] = f_far / (f_far - f_near);
  (*res)[3][2] = (-f_far * f_near) / (f_far - f_near);
  (*res)[2][3] = 1.0;
}

static void create_rotation_m(mat4x4 *res, double a, double b, double y) {
  for (int i = 0; i < 4; i++) {
    memset((*res)[i], 0, 4 * sizeof(double));
  }

  (*res)[0][0] = cos(b) * cos(y);
  (*res)[0][1] = sin(a) * sin(b) * cos(y) - cos(a) * sin(y);
  (*res)[0][2] = cos(a) * sin(b) * cos(y) + sin(a) * sin(y);
  (*res)[1][0] = cos(b) * sin(y);
  (*res)[1][1] = sin(a) * sin(b) * sin(y) + cos(a) * cos(y);
  (*res)[1][2] = cos(a) * sin(b) * sin(y) - sin(a) * cos(y);
  (*res)[2][0] = -sin(b);
  (*res)[2][1] = sin(a) * cos(b);
  (*res)[2][2] = cos(a) * cos(b);

  (*res)[3][3] = 1.0;
}

static void create_camera_point(vec3d *point, double x, double y, double z) {
  point->x = x;
  point->y = y;
  point->z = z;
}

static void translate_vector(vec3d *p, vec3d *r, double x, double y, double z) {
  r->x = p->x + x;
  r->y = p->y + y;
  r->z = p->z + z;
}

static void translate_mesh(mesh3d *obj, double x, double y, double z) {
  for (int tri_index = 0; tri_index < obj->tris_count; tri_index++) {
    for (int point_i = 0; point_i < 3; point_i++) {
      translate_vector(&(obj->tris[tri_index][point_i]), &(obj->tris[tri_index][point_i]), x, y, z);
    }
  }
}

static void multiply_matrix_vector(vec3d *i, vec3d *o,  mat4x4 *m) {
  double w;

  o->x = i->x * (*m)[0][0] + i->y * (*m)[1][0] + i->z * (*m)[2][0] + (*m)[3][0];
  o->y = i->x * (*m)[0][1] + i->y * (*m)[1][1] + i->z * (*m)[2][1] + (*m)[3][1];
  o->z = i->x * (*m)[0][2] + i->y * (*m)[1][2] + i->z * (*m)[2][2] + (*m)[3][2];
  w    = i->x * (*m)[0][3] + i->y * (*m)[1][3] + i->z * (*m)[2][3] + (*m)[3][3];

  if (w != 0.0) {
    o->x /= w;
    o->y /= w;
    o->z /= w;
  }
}

static void multiply_mesh(mesh3d *obj, mat4x4 *m) {
   for (int tri_index = 0; tri_index < obj->tris_count; tri_index++) {
    for (int point_i = 0; point_i < 3; point_i++) {
      vec3d new_p;
      
      multiply_matrix_vector(&(obj->tris[tri_index][point_i]), &new_p, m);

      obj->tris[tri_index][point_i].x = new_p.x;
      obj->tris[tri_index][point_i].y = new_p.y;
      obj->tris[tri_index][point_i].z = new_p.z;
    }
  }
}

static void normalize_to_screen(vec3d *p) {
  p->x += 1.0;
  p->y += 1.0;
  p->x *= 0.5 * (double)WIDTH;
  p->y *= 0.5 * (double)HEIGHT;
}

double dot_vec(vec3d *x, vec3d *y) {
  return x->x * y->x + x->y * y->y + x->z * y->z;
}

double vec_length(vec3d *x) {
  return sqrt(dot_vec(x, x));
}

static void get_normal(vec3d *normal, vec3d *p1, vec3d *p2, vec3d *p3) {
  vec3d line1, line2;
  sub_vec(&line1, p2, p1);
  sub_vec(&line2, p3, p1);

  normal->x = line1.y * line2.z - line1.z * line2.y;
  normal->y = line1.z * line2.x - line1.x * line2.z;
  normal->z = line1.x * line2.y - line1.y * line2.x;

  double normal_l = vec_length(normal);
  normal->x /= normal_l;
  normal->y /= normal_l;
  normal->z /= normal_l;
}

mesh3d *obj;
mat4x4 *projection_m, *rotation_m;
vec3d *camera_point;
static void render_cube(void) {
  translate_mesh(obj, 0.0, 0.0, 1.7);

  for (int tri_index = 0; tri_index < obj->tris_count; tri_index++) {
    vec3d normal, camera_to_point;
    
    get_normal(
      &normal,
      &(obj->tris[tri_index][0]),
      &(obj->tris[tri_index][1]),
      &(obj->tris[tri_index][2])
    );

    sub_vec(
      &camera_to_point,
      &(obj->tris[tri_index][0]),
      camera_point
    );

    if (dot_vec(&normal, &camera_to_point) >= 0.0) {
      continue;
    }

    triangle3d projected_triangle;
    for (int point_i = 0; point_i < 3; point_i++) {
      multiply_matrix_vector(&(obj->tris[tri_index][point_i]), &(projected_triangle[point_i]), projection_m);
      normalize_to_screen(&(projected_triangle[point_i]));
    }

    triangle2d show_triangle;
    triangle_to_2d(&show_triangle, &projected_triangle);

    for (int point_i = 0; point_i < 3; point_i++) {
      int next_point_i = (point_i + 1) % 3;

      oled_write_line(&(show_triangle[point_i]), &(show_triangle[next_point_i]), true);
    }
  }

  translate_mesh(obj, 0.0, 0.0, -1.7);
}

void keyboard_pre_init_user(void) {
  obj = malloc(sizeof(mesh3d));
  load_mesh_from_obj(obj);

  projection_m = malloc(sizeof(mat4x4));
  create_projection_m(projection_m, 0.1, 10.0, 60.0);

  rotation_m = malloc(sizeof(mat4x4));
  create_rotation_m(rotation_m, PI / 18, PI / 27, PI / 90);

  camera_point = malloc(sizeof(vec3d));
  create_camera_point(camera_point, 0, 0, 0);

  translate_mesh(obj, -0.5, -0.5, -0.5);
}

bool oled_task_user(void) {
  oled_clear();
  oled_render();
  multiply_mesh(obj, rotation_m);
  render_cube();

  return false;
}

oled_rotation_t fdoled_init_user(oled_rotation_t rotation) {
  if (!is_keyboard_master()) {
    return OLED_ROTATION_180;  // flips the display 180 degrees if offhand
  }
  return rotation;
}
#endif // OLED_ENABLE
