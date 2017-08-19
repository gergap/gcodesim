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
#ifndef VOXELSPACE_H_NPCYSM3K
#define VOXELSPACE_H_NPCYSM3K

#include <stdlib.h>

struct voxel_pos {
    int x, y, z;
};

void voxel_pos_set(struct voxel_pos *pos, unsigned int x, unsigned int y, unsigned int z);
void voxel_pos_add(struct voxel_pos *result, struct voxel_pos *a, struct voxel_pos *b);

struct voxel_space {
    struct voxel_pos pos;
    size_t width;
    size_t height;
    size_t thickness;
    size_t size; /**< size in bytes */
    unsigned char *data;
};

int voxel_space_init(struct voxel_space *space, size_t w, size_t h, size_t t);
void voxel_space_clear(struct voxel_space *space);

void voxel_space_set_all(struct voxel_space *space);
void voxel_space_clr_all(struct voxel_space *space);
int voxel_space_set_xyz(struct voxel_space *space, struct voxel_pos *pos);
int voxel_space_clr_xyz(struct voxel_space *space, struct voxel_pos *pos);
int voxel_space_get_xyz(struct voxel_space *space, struct voxel_pos *pos);
int voxel_space_difference(struct voxel_space *space, struct voxel_space *other);

int voxel_space_to_ppm(struct voxel_space *space, const char *basename);
int voxel_space_to_pgm(struct voxel_space *space, const char *filename);
int voxel_space_to_d3f(struct voxel_space *space, const char *filename);

#endif /* end of include guard: VOXELSPACE_H_NPCYSM3K */

