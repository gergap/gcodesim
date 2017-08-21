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
#include "voxelspace.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#ifdef _WIN32
/* Building for Windows */
# ifdef __GNUC__
   /* with MinGw we can use built-in functions like on Linux */
#  define htons(x) __builtin_bswap16(x)
# else
   /* for Visual Studio (untested) */
#  include <winsock2.h>
# endif
#else
/* Building for Liunx and other POSIX systems */
# include <arpa/inet.h> /* for htons */
#endif

void voxel_pos_set(struct voxel_pos *pos, unsigned int x, unsigned int y, unsigned int z)
{
    pos->x = x;
    pos->y = y;
    pos->z = z;
}

void voxel_pos_add(struct voxel_pos *result, struct voxel_pos *a, struct voxel_pos *b)
{
    result->x = a->x+ b->x;
    result->y = a->y+ b->y;
    result->z = a->z+ b->z;
}

/**
 * Converts the 3D coordianate into a linear index.
 *
 * @param space The voxel space in which the transformation should occur.
 * @param pos The 3D coordiante.
 * @param byte_idx Byte offset of coordinate inside \c space.
 * @param bit_idx Bit index for voxel inside byte.
 *
 * @return Linear index of Voxel, which is a bit index.
 */
static int voxel_space_index(struct voxel_space *space, struct voxel_pos *pos, size_t *byte_idx, unsigned char *bit_idx)
{
    size_t voxel_per_layer = space->width * space->height;
    size_t index = pos->z * voxel_per_layer + pos->y * space->width + pos->x;
    /* range check */
    if (index/8 >= space->size) return -1;
    /* compute bit index inside byte */
    *bit_idx = index & 7;
    *byte_idx = index >> 3;
    return 0;
}

int voxel_space_init(struct voxel_space *space, size_t w, size_t h, size_t t)
{
    space->width     = w;
    space->height    = h;
    space->thickness = t;
    space->size      = w* h * t / 8;
    space->data      = malloc(space->size);
    if (space->data == NULL) return -1;
    voxel_space_clr_all(space);
    return 0;
}

void voxel_space_clear(struct voxel_space *space)
{
    if (space->data)
        free(space->data);
    memset(space, 0, sizeof(*space));
}


void voxel_space_set_all(struct voxel_space *space)
{
    memset(space->data, 0xff, space->size);
}

void voxel_space_clr_all(struct voxel_space *space)
{
    memset(space->data, 0, space->size);
}

int voxel_space_set_xyz(struct voxel_space *space, struct voxel_pos *pos)
{
    size_t index;
    unsigned char bit;
    int ret = voxel_space_index(space, pos, &index, &bit);
    if (ret != 0) return ret;

    space->data[index] |= (1 << bit);

    return 0;
}

int voxel_space_clr_xyz(struct voxel_space *space, struct voxel_pos *pos)
{
    size_t index;
    unsigned char bit;
    int ret = voxel_space_index(space, pos, &index, &bit);
    if (ret != 0) return ret;

    space->data[index] &= ~(1 << bit);

    return 0;
}

/**
 * Gets voxel status at given position.
 *
 * @param space The voxel space.
 * @param pos Coordinate of voxel in voxelspace.
 *
 * @return 1 if voxel is set, 0 if it is not set, -1 if the pos is out of range.
 */
int voxel_space_get_xyz(struct voxel_space *space, struct voxel_pos *pos)
{
    size_t index;
    unsigned char bit;
    int ret = voxel_space_index(space, pos, &index, &bit);
    if (ret != 0) return ret;

    if (space->data[index] & (1 << bit)) return 1;

    return 0;
}

/**
 * Computes the difference of the two given voxel spaces.
 * space = space - other
 *
 * @param space Space to operate on
 * @param other Space to subtract from \c space
 *
 * @return Zero on success, -1 if there is a range error.
 */
int voxel_space_difference(struct voxel_space *space, struct voxel_space *other)
{
    unsigned int x, y, z;
    struct voxel_pos a, b;

    for (z = 0; z < other->thickness; ++z) {
        for (y = 0; y < other->height; ++y) {
           for (x = 0; x < other->width; ++x) {
               voxel_pos_set(&b, x, y, z);
               voxel_pos_add(&a, &b, &other->pos);

               /* range check */
               if (a.x < 0 || a.x >= space->width) continue;
               if (a.y < 0 || a.y >= space->height) continue;
               if (a.z < 0 || a.z >= space->thickness) continue;

               if (voxel_space_get_xyz(other, &b)) {
                   voxel_space_clr_xyz(space, &a);
               }
           }
        }
    }

    return 0;
}

int voxel_space_layer_to_ppm(struct voxel_space *space, const char *filename, unsigned int layer)
{
    FILE *f;
    int x, y, i;
    struct voxel_pos pos;

    f = fopen(filename, "w");
    if (f == NULL) return -1;

    fprintf(f, "P1\n");
    fprintf(f, "%u %u\n", (unsigned int)space->width, (unsigned int)space->height);
    for (y = space->height-1, i = 0; y >= 0; --y) {
        for (x = 0; x < space->width; ++x) {
            voxel_pos_set(&pos, x, y, layer);
            if (voxel_space_get_xyz(space, &pos)) {
                fprintf(f, "1");
            } else {
                fprintf(f, "0");
            }
            i++;
            if (i == 70) {
                fprintf(f, "\n");
                i = 0;
            }
        }
    }
    fclose(f);

    return 0;
}

/**
 * Stores the given voxel space into a series of PPM files,
 * one file for earch layer.
 *
 * @param space
 * @param basename
 *
 * @return
 */
int voxel_space_to_ppm(struct voxel_space *space, const char *basename)
{
    char filename[PATH_MAX];
    unsigned int z;

    for (z = 0; z < space->thickness; ++z) {
        snprintf(filename, sizeof(filename), "%s%03u.ppm", basename, z);
        voxel_space_layer_to_ppm(space, filename, z);
    }

    return 0;
}

int voxel_space_to_pgm(struct voxel_space *space, const char *filename)
{
    FILE *f;
    int x, y, z, i;
    struct voxel_pos pos;

    f = fopen(filename, "w");
    if (f == NULL) return -1;

    /* print format P2=grayscale */
    fprintf(f, "P2\n");
    /* print resolution */
    fprintf(f, "%u %u\n", (unsigned int)space->width, (unsigned int)space->height);
    /* print number of gray values */
    fprintf(f, "%u\n", (unsigned int)space->thickness);
    /* print pixel data */
    for (y = space->height-1, i = 0; y >= 0; --y) {
        for (x = 0; x < space->width; ++x) {
            /* find topmost voxel for x/y => gray value */
            for (z = space->thickness-1; z > 0; --z) {
                voxel_pos_set(&pos, x, y, z);
                if (voxel_space_get_xyz(space, &pos)) break;
            }
            fprintf(f, "%u ", z);
            i++;
            /* line wrap at max X, or at 70, whatever comes first */
            if (i == 70 || i == space->width) {
                fprintf(f, "\n");
                i = 0;
            }
        }
    }
    fclose(f);

    return 0;
}

static void voxel_space_write_uint16(FILE *f, uint16_t val)
{
    val = htons(val);
    fwrite(&val, 2, 1, f);
}

static void voxel_space_write_byte(FILE *f, uint8_t val)
{
    fwrite(&val, 1, 1, f);
}

/**
 * Writes a d3f file for rendering with POVRay.
 *
 * @param space
 * @param filename
 *
 * @return
 */
int voxel_space_to_d3f(struct voxel_space *space, const char *filename)
{
    FILE *f;
    int x, y, z;
    struct voxel_pos pos;

    f = fopen(filename, "w");
    if (f == NULL) return -1;

    /* write d3f header, 3 16 bit values (big endian) for the size of the voxelspace.
     * http://www.povray.org/documentation/view/3.6.1/374/
     */
    voxel_space_write_uint16(f, space->width);
    voxel_space_write_uint16(f, space->height);
    voxel_space_write_uint16(f, space->thickness);
    /* print pixel data */
    for (z = 0; z < space->thickness; ++z) {
        for (y = 0; y < space->height; ++y) {
            for (x = 0; x < space->width; ++x) {
                voxel_pos_set(&pos, x, y, z);
                if (voxel_space_get_xyz(space, &pos)) {
                    voxel_space_write_byte(f, 255);
                } else {
                    voxel_space_write_byte(f, 0);
                }
            }
        }
    }
    fclose(f);

    return 0;
}

