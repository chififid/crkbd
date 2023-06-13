/*
Copyright 2019 @foostan
Copyright 2020 Drashna Jaelre <@drashna>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

#define PI 3.1415
#define WIDTH 128
#define HEIGHT 32
#define TRIS_COUNT 12

#define CREATE_VEC3_OPERATION_FUNCTION(FUNNAME, OP) \
  static void FUNNAME(vec3d *res, vec3d *x, vec3d *y) { \
    res->x = x->x OP y->x; \
    res->y = x->y OP y->y; \
    res->z = x->z OP y->z; \
  }

typedef struct vec3d {
  double x;
  double y;
  double z;
} vec3d;

typedef vec3d triangle3d[3];

typedef double mat4x4[4][4];

typedef struct mesh {
  vec3d translation;
  mat4x4 rotation;
} mesh;

static const float PROGMEM obj_points[TRIS_COUNT][9] = {
  { -1.00000, 1.00000, 1.00000,  -1.00000, -1.00000, -1.00000,  -1.00000, -1.00000, 1.00000 },
  { -1.00000, -1.00000, -1.00000,  1.00000, 1.00000, -1.00000,  1.00000, -1.00000, -1.00000 },
  { 1.00000, 1.00000, -1.00000,  1.00000, -1.00000, 1.00000,  1.00000, -1.00000, -1.00000 },
  { 1.00000, 1.00000, 1.00000,  -1.00000, -1.00000, 1.00000,  1.00000, -1.00000, 1.00000 },
  { 1.00000, -1.00000, -1.00000,  -1.00000, -1.00000, 1.00000,  -1.00000, -1.00000, -1.00000 },
  { 1.00000, 1.00000, -1.00000,  -1.00000, 1.00000, 1.00000,  1.00000, 1.00000, 1.00000 },
  { -1.00000, 1.00000, 1.00000,  -1.00000, 1.00000, -1.00000,  -1.00000, -1.00000, -1.00000 },
  { -1.00000, -1.00000, -1.00000,  -1.00000, 1.00000, -1.00000,  1.00000, 1.00000, -1.00000 },
  { 1.00000, 1.00000, -1.00000,  1.00000, 1.00000, 1.00000,  1.00000, -1.00000, 1.00000 },
  { 1.00000, 1.00000, 1.00000,  -1.00000, 1.00000, 1.00000,  -1.00000, -1.00000, 1.00000},
  { 1.00000, -1.00000, -1.00000,  1.00000, -1.00000, 1.00000,  -1.00000, -1.00000, 1.00000 },
  { 1.00000, 1.00000, -1.00000,  -1.00000, 1.00000, -1.00000,  -1.00000, 1.00000, 1.00000 },
};

// BASIC OPERATIONS

CREATE_VEC3_OPERATION_FUNCTION(sub_vec, -)

static void create_vector(vec3d *p, double x, double y, double z) {
  p->x = x;
  p->y = y;
  p->z = z;
}

static void translate_vector(vec3d *p, double x, double y, double z) {
  p->x = p->x + x;
  p->y = p->y + y;
  p->z = p->z + z;
}

static void translate_triangle(triangle3d *triangle, double x, double y, double z) {
  for (int point_i = 0; point_i < 3; point_i++) {
    translate_vector(&((*triangle)[point_i]), x, y, z);
  }
}

static double dot_vec(vec3d *x, vec3d *y) {
  return x->x * y->x + x->y * y->y + x->z * y->z;
}

static double vec_length(vec3d *x) {
  return sqrt(dot_vec(x, x));
}

static void multiply_matrix_vector(vec3d *res, vec3d *p, mat4x4 *m) {
  vec3d temp;

  temp.x = p->x * (*m)[0][0] + p->y * (*m)[1][0] + p->z * (*m)[2][0] + (*m)[3][0];
  temp.y = p->x * (*m)[0][1] + p->y * (*m)[1][1] + p->z * (*m)[2][1] + (*m)[3][1];
  temp.z = p->x * (*m)[0][2] + p->y * (*m)[1][2] + p->z * (*m)[2][2] + (*m)[3][2];
  double w = p->x * (*m)[0][3] + p->y * (*m)[1][3] + p->z * (*m)[2][3] + (*m)[3][3];

  if (w != 0.0) {
    temp.x /= w;
    temp.y /= w;
    temp.z /= w;
  }

  res->x = temp.x;
  res->y = temp.y;
  res->z = temp.z;
}

static void multiply_matrix_triangle(triangle3d *res, triangle3d *triangle, mat4x4 *m) {
  for (int point_i = 0; point_i < 3; point_i++) {
    multiply_matrix_vector(&((*res)[point_i]), &((*triangle)[point_i]), m);
  }
}

// RENDER CODE

static void load_triangle_from_obj(triangle3d *res, int tri_index) {
  for (int point_i = 0; point_i < 3; point_i++) {
    (*res)[point_i].x = pgm_read_float(
      &(obj_points[tri_index][3 * point_i + 0])
    );
    (*res)[point_i].y = pgm_read_float(
      &(obj_points[tri_index][3 * point_i + 1])
    );
    (*res)[point_i].z = pgm_read_float(
      &(obj_points[tri_index][3 * point_i + 2])
    );
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

static void normalize_to_screen(vec3d *p) {
  p->x += 1.0;
  p->y += 1.0;
  p->x *= 0.5 * (double)WIDTH;
  p->y *= 0.5 * (double)HEIGHT;
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

static void oled_write_line(vec3d *start_p, vec3d *end_p, bool on) {
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

mat4x4 *projection_m, *rotation_m;
vec3d *camera_point;
mesh *obj;
static void render_cube(void) {
  for (int tri_index = 0; tri_index < TRIS_COUNT; tri_index++) {
    triangle3d triangle;
    vec3d normal, camera_to_point;

    load_triangle_from_obj(&triangle, tri_index);

    multiply_matrix_triangle(&triangle, &triangle, &(obj->rotation));

    translate_triangle(
      &triangle,
      obj->translation.x,
      obj->translation.y,
      obj->translation.z
    );

    translate_triangle(&triangle, 0, 0, 3.5);
    
    get_normal(
      &normal,
      &(triangle[0]),
      &(triangle[1]),
      &(triangle[2])
    );

    sub_vec(
      &camera_to_point,
      &(triangle[0]),
      camera_point
    );

    if (dot_vec(&normal, &camera_to_point) >= 0.0) {
      continue;
    }

    for (int point_i = 0; point_i < 3; point_i++) {
      multiply_matrix_vector(&(triangle[point_i]), &(triangle[point_i]), projection_m);
      normalize_to_screen(&(triangle[point_i]));
    }

    for (int point_i = 0; point_i < 3; point_i++) {
      int next_point_i = (point_i + 1) % 3;

      oled_write_line(&(triangle[point_i]), &(triangle[next_point_i]), true);
    }
  }
}

void keyboard_pre_init_user(void) {
  obj = malloc(sizeof(mesh));

  create_vector(&(obj->translation), 0, 0, 0);

  projection_m = malloc(sizeof(mat4x4));
  create_projection_m(projection_m, 0.1, 10.0, 60.0);

  camera_point = malloc(sizeof(vec3d));
  create_camera_point(camera_point, 0, 0, 0);
}

int tick;
bool oled_task_user(void) {
  oled_clear();
  create_rotation_m(
    &(obj->rotation),
    (PI / 2),
    (PI / 54) * tick,
    (PI / 36) * tick
  );
  obj->translation.x = sin(((double)tick / 27) * PI);
  oled_render();
  render_cube();

  tick = (tick + 1) % 180;
  return false;
}

oled_rotation_t fdoled_init_user(oled_rotation_t rotation) {
  if (!is_keyboard_master()) {
    return OLED_ROTATION_180;  // flips the display 180 degrees if offhand
  }
  return rotation;
}
#endif // OLED_ENABLE
