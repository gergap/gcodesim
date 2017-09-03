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
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <getopt.h>
#include "voxelspace.h"
#include "gcode.h"
#include "version.h"
#ifdef __linux__
#include <signal.h>
#include <unistd.h>
#endif

/* By enabling this define the tool generates output for
 * rendering an animation using POVRay.
 */
//#define POVRAY_ANIM_OUTPUT

#define sqr(x) (x)*(x)
static float g_resolution = 0.05; /* mm */
/* eagle pcbcode can generate negative coordinates when not mirroring.
 * We compensate this by adding the PCB width when this flag is set.
 */
static int g_x_mirror = 0;
static float g_tool1_d = 2;
static float g_tool2_d = 0.8;
static unsigned int g_file_index = 0;

void create_etch_tool(struct voxel_space *tool, float diameter)
{
    int ret;
    float r, tool_r;
    unsigned int x, y, z;
    unsigned int w, h, t;
    struct voxel_pos pos;

    w = diameter / g_resolution;
    h = diameter / g_resolution;
    t = 1 / g_resolution;

    voxel_space_clear(tool);

    ret = voxel_space_init(tool, w, h, t);
    if (ret != 0) {
        fprintf(stderr, "Failed to init voxel space.\n");
        exit(EXIT_FAILURE);
    }
    printf("Initialized tool voxel space [%u,%u,%u]: %u KB\n", w, h, t, (unsigned int)(tool->size / 1024));

    for (z = 0; z < tool->thickness; ++z) {
        tool_r = z * g_resolution - 0.1;
        if (tool_r < 0) tool_r = 0;
        if (tool_r > (diameter/2.0)) tool_r = diameter/2.0;
        tool_r *= tool_r; /* square to avoid sqrt in inner loop */
        for (y = 0; y < tool->height; ++y) {
            for (x = 0; x < tool->width; ++x) {
                r = sqr(x - w/2) + sqr(y - h/2);
                r *= sqr(g_resolution);
                if (r <= tool_r) {
                    voxel_pos_set(&pos, x, y, z);
                    voxel_space_set_xyz(tool, &pos);
                }
            }
        }
    }
    tool->pos.x = 0;
    tool->pos.y = 0;
    tool->pos.z = 10 / g_resolution;
}

void create_drill_tool(struct voxel_space *tool, float diameter)
{
    int ret;
    float r, tool_r;
    unsigned int x, y, z;
    unsigned int w, h, t;
    struct voxel_pos pos;

    w = diameter / g_resolution;
    h = diameter / g_resolution;
    t = 1 / g_resolution;

    ret = voxel_space_init(tool, w, h, t);
    if (ret != 0) {
        fprintf(stderr, "Failed to init voxel space.\n");
        exit(EXIT_FAILURE);
    }
    printf("Initialized tool voxel space [%u,%u,%u]: %u KB\n", w, h, t, (unsigned int)(tool->size / 1024));

    tool_r = sqr(diameter/2);
    for (z = 0; z < tool->thickness; ++z) {
        for (y = 0; y < tool->height; ++y) {
            for (x = 0; x < tool->width; ++x) {
                r = sqr(x - w/2) + sqr(y - h/2);
                r *= sqr(g_resolution);
                if (r <= tool_r) {
                    voxel_pos_set(&pos, x, y, z);
                    voxel_space_set_xyz(tool, &pos);
                }
            }
        }
    }
    tool->pos.x = 0;
    tool->pos.y = 0;
    tool->pos.z = 10 / g_resolution;
}

static struct voxel_space g_workpart;
static struct voxel_space g_tool1;
static struct voxel_space g_tool2;
static struct voxel_space *g_tool = &g_tool1;
volatile int              g_terminate = 0;

#ifdef __linux__
void signal_handler(int signo)
{
    switch (signo) {
    case SIGALRM:
        /* save current state periodically */
        printf("Saving current state to workpart.pgm.\n");
        voxel_space_to_pgm(&g_workpart, "workpart.pgm");
        alarm(5);
        break;
    case SIGINT:
    case SIGTERM:
        /* save current state when program is terminated */
        printf("Receive signal %i. Terminating now.\n", signo);
        g_terminate = 1;
        break;
    }
}
#endif

void gcode_callback(struct gcode_ctx *ctx)
{
#ifdef POVRAY_ANIM_OUTPUT
    FILE *f;
    char filename[255];
    static unsigned int cnt = 0;
    static unsigned int frame = 0;
#endif
    struct voxel_pos bak;

    g_tool->pos.x = (ctx->pos.x / g_resolution);
    if (g_x_mirror) g_tool->pos.x += g_workpart.width;
    g_tool->pos.y = (ctx->pos.y / g_resolution);
    g_tool->pos.z = ((ctx->pos.z + 1.6) / g_resolution);
    // center tool before difference
    bak = g_tool->pos; // backup
    g_tool->pos.x -= g_tool->width/2;
    g_tool->pos.y -= g_tool->height/2;

    /* validate position */
    if (g_tool->pos.x < 0 || g_tool->pos.x >= g_workpart.width ||
        g_tool->pos.y < 0 || g_tool->pos.y >= g_workpart.height) {
        /* note: it is normal to be outside in Z axis */
        fprintf(stderr, "warning: position outside of workpart (%.04f/%.04f/%.04f)\n",
                ctx->pos.x, ctx->pos.y, ctx->pos.z);
    }

    /* cut out material */
    voxel_space_difference(&g_workpart, g_tool);
    g_tool->pos = bak; // restore

#ifdef POVRAY_ANIM_OUTPUT
    cnt++;
    /* render every 10th position */
    if (cnt == 10) {
        cnt = 0;
    } else {
        return;
    }

    snprintf(filename, sizeof(filename), "povray/pos%04u.inc", frame);
    f = fopen(filename, "w");
    if (f) {
        fprintf(f, "#declare pos = <%f,%f,%f>;\n",
                (float)g_tool->pos.x * g_resolution,
                ctx->pos.z,
                (float)g_tool->pos.y * g_resolution);
        fprintf(f, "#declare filename = \"workpart%04u.png\";\n", frame);
        if (g_tool == &g_tool1) {
            fprintf(f, "#declare toolfile = \"%u_tool1.d3f\";\n", g_file_index);
            fprintf(f, "#declare tooldiameter = %f;\n", g_tool1_d);
        } else {
            fprintf(f, "#declare toolfile = \"%u_tool2.d3f\";\n", g_file_index);
            fprintf(f, "#declare tooldiameter = %f;\n", g_tool2_d);
        }
        fclose(f);
    }

    snprintf(filename, sizeof(filename), "povray/workpart%04u.pgm", frame);
    voxel_space_to_pgm(&g_workpart, filename);

    frame++;
#endif
}

void gcode_toolchange_callback(unsigned int tool)
{
    switch (tool) {
    case 1:
        g_tool = &g_tool1;
        break;
    case 2:
        g_tool = &g_tool2;
        break;
    }
}

/**
 * Parsing tool info from PCBGcode generater header.
 *
 * Etch Tool Info:
 * (  Tool Size)
 * (0.2000 )
 * Drill Tool Info:
 * ( Tool|       Size       |  Min Sub |  Max Sub |   Count )
 * ( T01  0.914mm 0.0360in 0.0000in 0.0000in )
 * ( T02  1.016mm 0.0400in 0.0000in 0.0000in )
 *
 * @param filename
 */
int parse_headers(const char *filename)
{
    char line[1024];
    FILE *f = fopen(filename, "r");
    int state = 0;
    int n, tool;
    float diameter;

    if (f == NULL) return -1;

    while (fgets(line, sizeof(line), f)) {
        if (line[0] != '(') break;
        switch (state) {
        case 0:
            if (strcmp(line, "(  Tool Size)\n") == 0)
                state = 1; /* parse etch tool info */
            else if (strcmp(line, "( Tool|       Size       |  Min Sub |  Max Sub |   Count )\n") == 0)
                state = 2; /* parse etch tool info */
            break;
        case 1:
            n = sscanf(line, "(%f )", &diameter);
            if (n == 1) {
                printf("Creating T01 with d=%fmm\n", diameter);
                g_tool1_d = diameter;
                create_etch_tool(&g_tool1, diameter);
                create_drill_tool(&g_tool1, diameter);
            } else {
                //printf("Error in parsing etch tool info\n");
                state = 0;
            }
            break;
        case 2:
            n = sscanf(line, "( T%u  %fmm", &tool, &diameter);
            if (n == 2) {
                printf("Creating T%02u with d=%fmm\n", tool, diameter);
                switch (tool) {
                case 1:
                    g_tool1_d = diameter;
                    create_drill_tool(&g_tool1, diameter);
                    break;
                case 2:
                    g_tool2_d = diameter;
                    create_drill_tool(&g_tool2, diameter);
                    break;
                }
            } else {
                //printf("Error in parsing drill tool info\n");
                state = 0;
            }
            break;
        }
    }

    fclose(f);

    return 0;
}

void usage(const char *appname)
{
    fprintf(stderr, "GCodesim %s\n", TOOL_VERSION);
    fprintf(stderr, "Copyright (C) 2017 Gerhard Gappmeier\n");
    fprintf(stderr, "This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.\n");
    fprintf(stderr, "This is free software, and you are welcome to redistribute it\n");
    fprintf(stderr, "under certain conditions; type `show c' for details.\n\n");
    fprintf(stderr, "Usage: %s [options] file...\n", appname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h: Shows this help\n");
    fprintf(stderr, "  -v: Verbose output\n");
    fprintf(stderr, "  -r: Specifies size of one voxel in mm (default=0.1mm)\n");
    fprintf(stderr, "  -W: Specifies PCB width in mm (default=100mm)\n");
    fprintf(stderr, "  -H: Specifies PCB height in mm (default=80mm)\n");
    fprintf(stderr, "  -r: Specifies size of one voxel in mm (default=0.1mm)\n");
    fprintf(stderr, "  -t: Specifies tool index to use (if no tool selection is in the Gcode)\n");
    fprintf(stderr, "      arg: <tool_index>[:<diameter>[<type>]]\n");
    fprintf(stderr, "      example: -t 1:3d means tool1=drill tool with 3mm diamter\n");
    fprintf(stderr, "      diameter: unit mm\n");
    fprintf(stderr, "      type: d=drill tool, c=cone shaped etch tool (default=d)\n");
    fprintf(stderr, "  -o: Specifies output filname, to rewrite the given GCode files\n");
    fprintf(stderr, "  -c: Load custom header to insert in output file\n");
    fprintf(stderr, "  -x: Applies given offset in mm at X axis\n");
    fprintf(stderr, "  -y: Applies given offset in mm at Y axis\n");
    fprintf(stderr, "  -z: Applies given offset in mm at Z axis\n");
    fprintf(stderr, "  -m: PCBGcode mirror compensation. Makes the coordinates positive by adding the PCB width.\n");
    fprintf(stderr, "Example: ./gcodesim -W 30 -m -x-5 -o drill.gcode ~/eagle/isp_adapter/isp_adapter.bot.drill.gcode\n");
}

int main(int argc, char *argv[])
{
    int ret;
    char filename[PATH_MAX] = "test.gcode";
    char ofilename[PATH_MAX] = "output.gcode";
    char toolfilename[PATH_MAX] = "";
    char customfilename[PATH_MAX] = "";
    float w = 100; /* mm */
    float h = 80; /* mm */
    float t = 1.6; /* mm */
    float offset_x = 0;
    float offset_y = 0;
    float offset_z = 0;
    float diameter = 0;
    char tool_type = 'd'; /* d=drill, c=cone */
    unsigned int x;
    unsigned int y;
    unsigned int z;
    int opt;
    int tool;

    while ((opt = getopt(argc, argv, "hW:H:r:mt:x:y:z:o:vc:")) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            exit(0);
            break;
        case 'W':
            w = atoi(optarg);
            break;
        case 'H':
            h = atoi(optarg);
            break;
        case 'x':
            offset_x = atof(optarg);
            break;
        case 'y':
            offset_y = atof(optarg);
            break;
        case 'z':
            offset_z = atof(optarg);
            break;
        case 'r':
            g_resolution = atof(optarg);
            break;
        case 'm':
            g_x_mirror = 1;
            break;
        case 't':
            ret = sscanf(optarg, "%i:%f%c", &tool, &diameter, &tool_type);
            if (ret < 1) {
                fprintf(stderr, "error: could not parse tool option\n");
                exit(EXIT_FAILURE);
            }
            switch (tool) {
            case 1:
                g_tool = &g_tool1;
                g_tool1_d = diameter;
                break;
            case 2:
                g_tool = &g_tool2;
                g_tool1_d = diameter;
                break;
            default:
                fprintf(stderr, "error: there is not tool with index %i\n", tool);
                exit(EXIT_FAILURE);
            }
            if (ret >= 2) {
                if (tool_type == 'd') {
                    printf("Creating drill tool with d=%f mm.\n", diameter);
                    create_drill_tool(g_tool, diameter);
                } else {
                    printf("Creating etch tool (cone) with d=%f mm.\n", diameter);
                    create_etch_tool(g_tool, diameter);
                }
            }
            if (g_tool->data == NULL) {
                fprintf(stderr, "error: tool %i has not been created yet.\n", tool);
                exit(EXIT_FAILURE);
            }
            break;
        case 'o':
            strncpy(ofilename, optarg, sizeof(ofilename));
            gcode_set_output(ofilename);
            break;
        case 'c':
            strncpy(customfilename, optarg, sizeof(customfilename));
            ret = gcode_load_custom_header(customfilename);
            if (ret != 0) {
                fprintf(stderr, "error: loading custom header '%s' failed.\n", customfilename);
                exit(EXIT_FAILURE);
            }
            break;
        case 'v':
            gcode_verbose();
            break;
        default: /* '?' */
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    while (argc <= optind) {
        fprintf(stderr, "error: No filename was given.\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    gcode_set_offset(offset_x, offset_y, offset_z);

    x = w / g_resolution;
    y = h / g_resolution;
    z = t / g_resolution;
    ret = voxel_space_init(&g_workpart, x, y, z);
    if (ret != 0) {
        fprintf(stderr, "error: Failed to init voxel space.\n");
        exit(EXIT_FAILURE);
    }
    printf("Initialized voxel space [%u,%u,%u]: %u MB\n", x, y, z, (unsigned int)(g_workpart.size / 1024 / 1024));

    //create_etch_tool(&g_tool1, g_tool1_d);
    //create_drill_tool(&g_tool1, 0.15);
    //create_drill_tool(&g_tool1, 0.9);
    //create_drill_tool(&g_tool2, 1);
    //create_drill_tool(&g_tool2, g_tool2_d);
    //voxel_space_to_ppm(&g_tool1, "etch");

    voxel_space_set_all(&g_workpart);
#ifdef POVRAY_ANIM_OUTPUT
    voxel_space_to_pgm(&g_workpart, "povray/workpart0000.pgm");
#endif

#ifdef __linux__
    /* install some signal handlers */
    signal(SIGALRM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    alarm(5);
#endif


    /* parse all given gcode files */
    g_file_index = 1;
    while (argc > optind) {
        strncpy(filename, argv[optind++], sizeof(filename));
        ret = parse_headers(filename);
        if (ret != 0) {
            fprintf(stderr, "error: could not open '%s'.\n", filename);
            exit(EXIT_FAILURE);
        }

#ifdef POVRAY_ANIM_OUTPUT
        /* export tools for this file */
        snprintf(toolfilename, sizeof(toolfilename), "povray/%u_tool1.d3f", g_file_index);
        voxel_space_to_d3f(&g_tool1, toolfilename);
        snprintf(toolfilename, sizeof(toolfilename), "povray/%u_tool2.d3f", g_file_index);
        voxel_space_to_d3f(&g_tool2, toolfilename);
#endif

        gcode_parse(filename, gcode_callback, gcode_toolchange_callback);
        g_file_index++;
    }
    printf("Saving result to workpart.pgm.\n");
    voxel_space_to_pgm(&g_workpart, "workpart.pgm");
    //voxel_space_to_d3f(&g_workpart, "workpart.d3f");

    voxel_space_clear(&g_workpart);
    voxel_space_clear(&g_tool1);
    voxel_space_clear(&g_tool2);

    return 0;
}
