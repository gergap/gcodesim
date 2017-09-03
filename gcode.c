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
#include "gcode.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define sqr(x) (x)*(x)

static FILE *g_output = NULL;
static const char *g_ofilename = NULL;
static float g_offset_x, g_offset_y, g_offset_z;
static int   g_verbose = 0;
static int   g_header_written = 0;

/* Write custom header after G90 has been detected. */
static char g_custom_header[4096] = "";

enum gcode_arc_mode {
    ARC_NONE = 0,
    ARC_CW,
    ARC_CCW
};

/**
 * Verbose output.
 *
 * @param verbosity level
 * @param fmt Printf format specifier.
 * @param ...
 */
static void verbose(int level, const char *fmt, ...)
{
    va_list ap;

    if (g_verbose < level) return;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

/**
 * Increase verbose output level.
 */
void gcode_verbose(void)
{
    g_verbose++;
}

void gvector_add(struct gvector *res, struct gvector *a, struct gvector *b)
{
    res->x = a->x + b->x;
    res->y = a->y + b->y;
    res->z = a->z + b->z;
}

void gvector_sub(struct gvector *res, struct gvector *a, struct gvector *b)
{
    res->x = a->x - b->x;
    res->y = a->y - b->y;
    res->z = a->z - b->z;
}

void gvector_mul(struct gvector *v, float factor)
{
    v->x *= factor;
    v->y *= factor;
    v->z *= factor;
}

float gvector_len(struct gvector *v)
{
    float len = sqr(v->x) + sqr(v->y) + sqr(v->z);
    len = sqrt(len);
    return len;
}

void gcode_ctx_init(struct gcode_ctx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->pos_absolute = true;
}

static void gcode_send_pos_cb(struct gcode_ctx *ctx)
{
    verbose(3, "%s: X%.2f, Y%.2f, Z%.2f\n", __func__, ctx->pos.x, ctx->pos.y, ctx->pos.z);

    if (ctx->newpos_cb) ctx->newpos_cb(ctx);
}

int gcode_linear_move(struct gcode_ctx *ctx, struct gvector *newpos)
{
    int ret = 0;
    struct gvector diff, step;
    float len, step_len = 0.05;
    unsigned int i, num_steps;

    gvector_sub(&diff, newpos, &ctx->pos);
    len = gvector_len(&diff);
    if (len == 0) return 0;
    step = diff;
    gvector_mul(&step, step_len / len);

    //printf("len=%f\n", len);
    num_steps = len / step_len;
    verbose(2, "num_steps=%u\n", num_steps);

    if (num_steps > 0) {
        for (i = 0; i < num_steps-1; ++i) {
            gvector_add(&ctx->pos, &ctx->pos, &step);
            gcode_send_pos_cb(ctx);
        }
    }
    ctx->pos = *newpos;
    gcode_send_pos_cb(ctx);

    return ret;
}

int gcode_arc_move(struct gcode_ctx *ctx, struct gvector *endpos, struct gvector *center, enum gcode_arc_mode mode)
{
    int ret = 0;
    struct gvector startpos = ctx->pos, v;
    float radius = gvector_len(center);
    float len = radius * 2 * M_PI;
    float step_len = 0.05;
    float s_alpha, e_alpha, d_alpha; /* start and end angles */
    float alpha;
    float dx, dy;
    unsigned int i, num_steps;

    verbose(2, "start pos =%.04f/%.04f/%.04f\n", startpos.x, startpos.y, startpos.z);
    verbose(2, "end pos   =%.04f/%.04f/%.04f\n", endpos->x, endpos->y, endpos->z);

    /* compute absolute center */
    gvector_add(center, center, &startpos);
    verbose(2, "center    =%.04f/%.04f/%.04f\n", center->x, center->y, center->z);

    /* compute number of steps (for full circle) */
    num_steps = len / step_len;
    verbose(2, "num_steps=%u\n", num_steps);

    /* compute start angle */
    dx = startpos.x - center->x;
    dy = startpos.y - center->y;
    s_alpha = atan2(dy, dx);
    printf("start: dx=%f, dy=%f, alpha=%f (%f°)\n", dx, dy, s_alpha, RAD2DEG(s_alpha));
    /* compute end angle */
    dx = endpos->x - center->x;
    dy = endpos->y - center->y;
    e_alpha = atan2(dy, dx);
    printf("end: dx=%f, dy=%f, alpha=%f (%f°)\n", dx, dy, e_alpha, RAD2DEG(e_alpha));
    /* compute steps angle: not atan2 returns a range from -PI:PI */
    if (mode == ARC_CW) {
        d_alpha = (s_alpha - e_alpha);
    } else {
        d_alpha = (e_alpha - s_alpha);
    }
    printf("d_alpha=%f (%f°)\n", d_alpha, RAD2DEG(d_alpha));
    if (d_alpha < 0) {
        d_alpha += 2*M_PI;
        printf("d_alpha(corrected)=%f (%f°)\n", d_alpha, RAD2DEG(d_alpha));
    }
    /* correct number of steps from 360° to alpha */
    num_steps = num_steps * RAD2DEG(d_alpha) / 360;
    /* compute step angle */
    d_alpha /= num_steps;
    if (mode == ARC_CW) d_alpha = -d_alpha;

    alpha = s_alpha;
    for (i = 1; i < num_steps; ++i) {
        alpha += d_alpha;
        v.x = radius * cos(alpha);
        v.y = radius * sin(alpha);
        v.z = 0;
        gvector_add(&ctx->pos, center, &v);
        gcode_send_pos_cb(ctx);
    }
    ctx->pos = *endpos;
    gcode_send_pos_cb(ctx);

    return ret;
}

int gcode_parse_float(const char *line, const char *key, float *val)
{
    char *find = strstr(line, key);
    int ret;

    if (find == NULL) return -1;

    find++;
    ret = sscanf(find, "%f", val);
    if (ret == 1) return 0;
    return -1;
}

#define APPEND(fmt, arg) ret = snprintf(newline + pos, sizeof(newline) - pos, fmt, arg); \
                               if (ret > 0) { \
                                   pos += ret; \
                               }

int gcode_parse_gcode(struct gcode_ctx *ctx, const char *line)
{
    int ret = 0;
    unsigned int code;
    struct gvector newpos = ctx->pos;
    struct gvector center;
    enum gcode_arc_mode mode = ARC_NONE;
    float val;
    char newline[256];
    int pos = 0;

    ret = sscanf(line, "G%u", &code);
    if (ret != 1) goto error;

    switch (code) {
    case 0: /* rapid move */
    case 1: /* linear move */
        APPEND("G%02u", code);
        if (gcode_parse_float(line, "X", &val) == 0) {
            if (ctx->pos_absolute) {
                newpos.x = val + g_offset_x;
                APPEND(" X%.4f", newpos.x);
            } else {
                newpos.x += val;
            }
        }
        if (gcode_parse_float(line, "Y", &val) == 0) {
            if (ctx->pos_absolute) {
                newpos.y = val + g_offset_y;
                APPEND(" Y%.4f", newpos.y);
            } else {
                newpos.y += val;
            }
        }
        if (gcode_parse_float(line, "Z", &val) == 0) {
            if (ctx->pos_absolute) {
                newpos.z = val + g_offset_z;
                APPEND(" Z%.4f", newpos.z);
            } else {
                newpos.z += val;
            }
        }
        if (gcode_parse_float(line, "F", &val) == 0) {
            ctx->feedrate = val;
            APPEND(" F%.2f", val);
        }
        verbose(2, "Linear move to X=%.2f, Y=%.2f, Z=%.2f\n",
                newpos.x, newpos.y, newpos.z);
        gcode_linear_move(ctx, &newpos);
        if (ctx->pos_absolute) {
            /* add offset */
            if (g_output) fprintf(g_output, "%s\n", newline);
        } else {
            /* relative pos, take as-is */
            if (g_output) fprintf(g_output, "%s", line);
        }
        break;
    case 2: /* arc CW */
        mode = ARC_CW;
        /* implicit fall-through */
    case 3: /* arc CCW */
        if (mode == ARC_NONE) mode = ARC_CCW;
        APPEND("G%02u", code);
        if (gcode_parse_float(line, "X", &val) == 0) {
            if (ctx->pos_absolute) {
                newpos.x = val + g_offset_x;
                APPEND(" X%.4f", newpos.x);
            } else {
                newpos.x += val;
            }
        }
        if (gcode_parse_float(line, "Y", &val) == 0) {
            if (ctx->pos_absolute) {
                newpos.y = val + g_offset_y;
                APPEND(" Y%.4f", newpos.y);
            } else {
                newpos.y += val;
            }
        }
        if (gcode_parse_float(line, "Z", &val) == 0) {
            if (ctx->pos_absolute) {
                newpos.z = val + g_offset_z;
                APPEND(" Z%.4f", newpos.z);
            } else {
                newpos.z += val;
            }
        }
        /* note that IJK is always relative to starting pos */
        center.x = 0;
        center.y = 0;
        center.z = 0;
        if (gcode_parse_float(line, "I", &val) == 0) {
            APPEND(" I%.4f", val);
            center.x = val;
        }
        if (gcode_parse_float(line, "J", &val) == 0) {
            APPEND(" J%.4f", val);
            center.y = val;
        }
        if (gcode_parse_float(line, "K", &val) == 0) {
            APPEND(" K%.4f", val);
            center.z = val;
        }
        if (gcode_parse_float(line, "F", &val) == 0) {
            ctx->feedrate = val;
            APPEND(" F%.2f", val);
        }
        verbose(2, "Arc to X=%.2f, Y=%.2f, Z=%.2f (rel. center=%.2f/%.2f/%.2f)\n",
                newpos.x, newpos.y, newpos.z, center.x, center.y, center.z);
        gcode_arc_move(ctx, &newpos, &center, mode);
        mode = ARC_NONE; /* reset mode */
        if (ctx->pos_absolute) {
            /* add offset */
            if (g_output) fprintf(g_output, "%s\n", newline);
        } else {
            /* relative pos, take as-is */
            if (g_output) fprintf(g_output, "%s", line);
        }
        break;
    case 4: /* Dwell */
        verbose(1, "Pausing ignored. We want to do a quick simulation.\n");
        break;
    case 20: /* inch */
        verbose(1, "Units set to inch\n");
        break;
    case 21: /* mm */
        verbose(1, "Units set to mm\n");
        break;
    case 28: /* homeing */
        verbose(1, "Homeing\n");
        break;
    case 90: /* position absolute */
        verbose(1, "Positioning absolute\n");
        ctx->pos_absolute = true;
        break;
    case 91: /* position relative */
        verbose(1, "Positioning relative\n");
        ctx->pos_absolute = false;
        break;
    case 92: /* set position */
        APPEND("G%02u", code);
        if (gcode_parse_float(line, "X", &val) == 0) {
            if (ctx->pos_absolute) {
                newpos.x = val + g_offset_x;
                APPEND(" X%.4f", newpos.x);
            } else {
                newpos.x += val;
            }
        }
        if (gcode_parse_float(line, "Y", &val) == 0) {
            if (ctx->pos_absolute) {
                newpos.y = val + g_offset_y;
                APPEND(" Y%.4f", newpos.y);
            } else {
                newpos.y += val;
            }
        }
        if (gcode_parse_float(line, "Z", &val) == 0) {
            if (ctx->pos_absolute) {
                newpos.z = val + g_offset_z;
                APPEND(" Z%.4f", newpos.z);
            } else {
                newpos.z += val;
            }
        }
        ctx->pos = newpos;
        verbose(1, "Setting position to X=%.2f, Y=%.2f, Z=%.2f\n",
                newpos.x, newpos.y, newpos.z);
        if (ctx->pos_absolute) {
            /* add offset */
            if (g_output) fprintf(g_output, "%s\n", newline);
        } else {
            /* relative pos, take as-is */
            if (g_output) fprintf(g_output, "%s", line);
        }
        break;
    default:
        verbose(1, "ignoring: %s\n", line);
        break;
    }

    if (g_output && code != 0 && code != 1 && code != 2 && code != 3 && code != 92) {
        fprintf(g_output, "%s", line);
        if (code == 90 && g_header_written == 0) {
            fprintf(g_output, "%s", g_custom_header);
            g_header_written = 1;
        }
    }

    return ret;
error:
    printf("error in %s: %i\n", __func__, ret);
    return -1;
}

int gcode_parse_mcode(struct gcode_ctx *ctx, const char *line)
{
    int ret = 0;
    unsigned int code, tool;
    struct gvector newpos = ctx->pos;
    float val;

    ret = sscanf(line, "M%u", &code);
    if (ret != 1) goto error;

    switch (code) {
    case 2: /* program off */
        verbose(1, "program off\n");
        break;
    case 3: /* spindle on CW */
        verbose(1, "spindle on CW\n");
        break;
    case 4: /* spindle on CCW */
        verbose(1, "spindle on CCW\n");
        break;
    case 5: /* spindle off */
        verbose(1, "spindle off\n");
        break;
    case 6: /* tool chain */
        ret = sscanf(line, "M%u T%u", &code, &tool);
        if (ret != 2) goto error;
        verbose(1, "select tool %u\n", tool);
        if (ctx->toolchange_cb) ctx->toolchange_cb(tool);
        break;
    default:
        verbose(1, "ignoring: %s\n", line);
        break;
    }

    if (g_output) {
        fprintf(g_output, "%s", line);
    }

    return ret;
error:
    printf("error in %s: %i\n", __func__, ret);
    return -1;
}

int gcode_parse_tcode(struct gcode_ctx *ctx, const char *line)
{
    int ret = 0;
    unsigned int code;
    struct gvector newpos = ctx->pos;
    float val;

    ret = sscanf(line, "T%u", &code);
    if (ret != 1) goto error;

    verbose(1, "select tool %u\n", code);

    if (g_output) {
        fprintf(g_output, "%s", line);
    }

    return ret;
error:
    printf("error in %s: %i\n", __func__, ret);
    return -1;
}

int gcode_parse_line(struct gcode_ctx *ctx, const char *line)
{
    int ret = 0;

    switch (line[0]) {
    case 'G':
        ret = gcode_parse_gcode(ctx, line);
        break;
    case 'M':
        ret = gcode_parse_mcode(ctx, line);
        break;
    case 'T':
        ret = gcode_parse_tcode(ctx, line);
        break;
    case '(':
    case ';':
        /* ignore comments */
        if (g_output) fprintf(g_output, "%s", line);
        break;
    case 0:
    case '\n':
        /* ignore emtpy string */
        if (g_output) fprintf(g_output, "\n");
        break;
    default:
        verbose(1, "Unknown code: %s\n", line);
        ret = -1;
        break;
    }

    return ret;
}

int gcode_parse(const char *filename, void (*newpos_cb)(struct gcode_ctx *ctx), void (*toolchange_cb)(unsigned int tool))
{
    char line[4096];
    char *result;
    FILE *f;
    unsigned int code;
    float x, y, z, F;
    int ret;
    struct gcode_ctx ctx;

    f = fopen(filename, "r");
    if (f == NULL) return -1;

    if (g_ofilename) {
        g_output = fopen(g_ofilename, "w");
        if (g_output == NULL) {
            fclose(f);
            return -1;
        }
    }

    gcode_ctx_init(&ctx);
    ctx.newpos_cb     = newpos_cb;
    ctx.toolchange_cb = toolchange_cb;

    while (1) {
        result = fgets(line, sizeof(line), f);
        if (result == NULL) break;

        ret = gcode_parse_line(&ctx, line);
    }

    fclose(f);
    if (g_output) {
        fclose(g_output);
        g_output = NULL;
    }

    return 0;
}

void gcode_set_output(const char *filename)
{
    g_ofilename = filename;
}

int gcode_load_custom_header(const char *filename)
{
    ssize_t ret;
    FILE *f = fopen(filename, "r");
    if (f == NULL) return -1;

    ret = fread(g_custom_header, 1, sizeof(g_custom_header), f);

    fclose(f);

    return 0;
}

void gcode_set_offset(float x, float y, float z)
{
    g_offset_x = x;
    g_offset_y = y;
    g_offset_z = z;
}

