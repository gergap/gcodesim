/*
 * GCode Simulator
 * Copyright (C) 2017 Gerhard Gappmeier

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef GCODE_H_MQ1JX08Y
#define GCODE_H_MQ1JX08Y

#include <stdbool.h>

#define RAD2DEG(x) ((x) * 180.0 / M_PI)
#define DEG2RAD(x) ((x) * M_PI / 180.0)

struct gvector {
    float x, y, z;
};

void gvector_add(struct gvector *res, struct gvector *a, struct gvector *b);
void gvector_sub(struct gvector *res, struct gvector *a, struct gvector *b);
void gvector_mul(struct gvector *v, float factor);
float gvector_len(struct gvector *v);

struct gcode_ctx {
    struct gvector pos;
    bool pos_absolute;
    float feedrate; /* mm/min */
    void (*newpos_cb)(struct gcode_ctx *ctx);
    void (*toolchange_cb)(unsigned int tool);
};

void gcode_ctx_init(struct gcode_ctx *ctx);

int gcode_parse(const char *filename, void (*newpos_cb)(struct gcode_ctx *ctx), void (*toolchange_cb)(unsigned int tool));
void gcode_set_output(const char *filename);
int gcode_load_custom_header(const char *filename);
void gcode_set_offset(float x, float y, float z);
void gcode_verbose(void);

#endif /* end of include guard: GCODE_H_MQ1JX08Y */

