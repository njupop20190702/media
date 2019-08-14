#include <stdio.h>
#include <malloc.h>
#include <string.h>

#define POOL_COUNT 3
#define PATH "/home/zhuhai/frame/frame"

#define FORAMT_NV12 0
#define FORAMT_I420 1

typedef struct pixel_color{
    unsigned char y;
    unsigned char u;
    unsigned char v;
} pixel_color_t;

typedef struct frame_buf {
    char *buf;
    unsigned long buf_size;
    unsigned int pic_w;
    unsigned int pic_h;
    unsigned int frame_index;
} frame_buf_t;

pixel_color_t g_palette[6];

static void init_palette(void) {
    g_palette[0].y = 178;
    g_palette[0].u = 135;
    g_palette[0].v = 169;

    g_palette[1].y = 76;
    g_palette[1].u = 111;
    g_palette[1].v = 119;

    g_palette[2].y = 184;
    g_palette[2].u = 145;
    g_palette[2].v = 118;

    g_palette[3].y = 128;
    g_palette[3].u = 182;
    g_palette[3].v = 47;

    g_palette[4].y = 199;
    g_palette[4].u = 109;
    g_palette[4].v = 149;

    g_palette[5].y = 178;
    g_palette[5].u = 105;
    g_palette[5].v = 116;
}

static int get_palette_index(unsigned int pic_w, unsigned int pic_h, unsigned long pixel_index, unsigned char frame_index) {
    int index = 0;
    index = (pixel_index % pic_w) / 128 + ((pixel_index / pic_w / 64) & 1) * 3;
    index = index + (frame_index) % 6;
    index = index % 6;
    return index;
}

static int draw_frame(frame_buf_t *frame) {
    unsigned long luma_pixel_count = 0, i = 0, j = 0;
    unsigned char *y = NULL, *uv = NULL;
    int palette_indx = 0;
    int uv_index = 0;

    if (!frame || !frame->buf) {
        printf("[%s]err:args invalid, frame(%p) buf(%p)\n", __func__, frame, frame->buf);
        return -1;
    }

    luma_pixel_count = frame->pic_w*frame->pic_h;
    y = frame->buf;
    uv = frame->buf + luma_pixel_count;
    int v_offset = 0;

    v_offset = luma_pixel_count>>2;

    for (i=0;i<luma_pixel_count;i++) {
        palette_indx = get_palette_index(frame->pic_w, frame->pic_h, i, frame->frame_index);

        y[i] = g_palette[palette_indx].y;
#if FORAMT_NV12
        if (i%4 == 0) {
            uv[uv_index++] = g_palette[palette_indx].u;
            uv[uv_index++] = g_palette[palette_indx].v;
        }
#elif FORAMT_I420
		if (i%4 == 0) {
            uv[uv_index+1] = g_palette[palette_indx].u;
            uv[uv_index + v_offset] = g_palette[palette_indx].v;
            uv_index++;
        }
#endif
    }
    return 0;
}

static int frame_pool_init(frame_buf_t *pool, unsigned int pic_w, unsigned int pic_h, unsigned char frame_total) {
    int i = 0;
    unsigned long space_size = 0;

    if (!pool || !frame_total) {
        return -1;
    }

    memset(pool, 0, frame_total*sizeof(frame_buf_t));
    space_size = pic_w*pic_h*3>>1;

    for (i=0;i<frame_total;i++) {
        pool->frame_index = i;
        pool->pic_w = pic_w;
        pool->pic_h = pic_h;
        pool->buf_size = space_size;
        pool->buf = (char *)malloc(space_size);
        if (!pool->buf) {
            printf("[%s]malloc buffer fail!\n", __func__);
            goto fail;
        }
        pool++;
    }
    return 0;
fail:
    for (i=0;i<frame_total;i++) {
        if (pool->buf) {
            free(pool->buf);
            pool->buf = NULL;
    }

    }
    return -1;
}

static void frame_pool_deinit(frame_buf_t *pool, unsigned char total_frame) {
    int i = 0;

    if (!pool || !total_frame)
        return;
    for (i=0;i<total_frame;i++) {
        if (pool->buf) {
            free(pool->buf);
            pool->buf = NULL;
        }
    }
}

static int dump_frame_start(FILE *fd, char *path, frame_buf_t *frame) {
    if (!frame || !frame->buf || (!fd && !path)) {
        printf("dump image fail, args invalid!\n");
        return -1;
    }

    if (!fd)
        fd = fopen(path, "wb");
    if (!fd) {
        printf("dump image fail, can not open (%s)\n", path);
        return -1;
    }

    fwrite(frame->buf, frame->buf_size, 1, fd);
}

static void dump_frame_end(FILE *fd) {
    if (fd)
        fclose(fd);
}

int main(void) {
    int i = 0;
    frame_buf_t frame_pool[POOL_COUNT];
    FILE *fd = NULL;
    char path[256];

    init_palette();

    frame_pool_init(frame_pool, 640, 480, POOL_COUNT);

    for (i=0;i<POOL_COUNT;i++) {
        draw_frame(frame_pool + i);
        sprintf(path, PATH"%d", i);
        dump_frame_start(fd, path, frame_pool + i);
        dump_frame_end(fd);
    }

    frame_pool_deinit(frame_pool, POOL_COUNT);
    return 0;
}


