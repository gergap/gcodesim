#include "../gcode.c"
#include <math.h>
#include <stdlib.h>

static struct gvector g_test_data[360];
static struct gvector g_center = { 50, 50, 0 };
static int g_start, g_end, g_control, g_mode;
volatile int g_terminate = 0;

static int gvector_fuzzy_compare(struct gvector *a, struct gvector *b, float epsilon)
{
    if (fabs(a->x - b->x) > epsilon) return 1;
    if (fabs(a->y - b->y) > epsilon) return 1;
    if (fabs(a->z - b->z) > epsilon) return 1;
    return 0;
}

static void prepare_test_data(void)
{
    unsigned int i;
    float a;
    struct gvector v = { 0, 0, 0 };
    float r = 40;

    for (i = 0; i < 360; ++i) {
        a = DEG2RAD(i);
        v.x = r * cos(a);
        v.y = r * sin(a);
        gvector_add(&g_test_data[i], &g_center, &v);
    }
}

static void test_newpos(struct gcode_ctx *ctx)
{
#if 0
    printf("control point %.04f/%.04f/%.04f\n", 
            g_test_data[g_control].x,
            g_test_data[g_control].y,
        g_test_data[g_control].z);
#endif

    if (gvector_fuzzy_compare(&ctx->pos, &g_test_data[g_control], 0.1) == 0) {
        printf("Reached control point %i\n", g_control);
        if (g_control != g_end) {
            g_control += g_mode;
            if (g_control < 0) g_control = 359;
            if (g_control > 359) g_control = 0;
        }
    }
}

int test_cw(int start, int end)
{
    struct gcode_ctx ctx;
    struct gvector center; // relative center JK

    gcode_ctx_init(&ctx);
    ctx.newpos_cb = test_newpos;
    ctx.pos = g_test_data[start];

    g_mode = -1;
    g_start = start;
    g_end = end;
    g_control = start - 1;
    if (g_control < 0) g_control = 359;

    fprintf(stdout, "Start Testcase: start=%i, end=%i\n", start, end);
    gvector_sub(&center, &g_center, &g_test_data[start]);
    gcode_arc_move(&ctx, &g_test_data[end], &center, ARC_CW);
    if (g_control == end)
        return 0;

    fprintf(stdout, "Testcase: start=%i, end=%i\n", start, end);
    fprintf(stdout, "Controll point %i not reached.\n", g_control);
    return -1;
}

int test_ccw(int start, int end)
{
    struct gcode_ctx ctx;
    struct gvector center; // relative center JK

    gcode_ctx_init(&ctx);
    ctx.newpos_cb = test_newpos;
    ctx.pos = g_test_data[start];

    g_mode = 1;
    g_start = start;
    g_end = end;
    g_control = start + 1;
    if (g_control > 359) g_control = 0;

    fprintf(stdout, "Start Testcase: start=%i, end=%i\n", start, end);
    gvector_sub(&center, &g_center, &g_test_data[start]);
    gcode_arc_move(&ctx, &g_test_data[end], &center, ARC_CCW);
    if (g_control == end)
        return 0;

    fprintf(stdout, "Testcase: start=%i, end=%i\n", start, end);
    fprintf(stdout, "Controll point %i not reached.\n", g_control);
    return -1;
}

int main(int argc, char *argv[])
{
    int ret;
    int exit_code = EXIT_SUCCESS;

    prepare_test_data();

    gcode_verbose();
    gcode_verbose();

    // CW, start<end
    ret = test_cw(0, 90);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_cw(90, 180);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_cw(180, 270);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_cw(270, 0);
    if (ret != 0) exit_code = EXIT_FAILURE;
    // CW, start>end
    ret = test_cw(90, 0);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_cw(180, 90);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_cw(270, 180);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_cw(0, 270);
    if (ret != 0) exit_code = EXIT_FAILURE;

    // CCW, start<end
    ret = test_ccw(0, 90);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_ccw(90, 180);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_ccw(180, 270);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_ccw(270, 0);
    if (ret != 0) exit_code = EXIT_FAILURE;
    // CCW, start>end
    ret = test_ccw(90, 0);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_ccw(180, 90);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_ccw(270, 180);
    if (ret != 0) exit_code = EXIT_FAILURE;
    ret = test_ccw(0, 270);
    if (ret != 0) exit_code = EXIT_FAILURE;


    return exit_code;
}
