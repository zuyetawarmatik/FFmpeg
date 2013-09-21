/*
 * Copyright (c) 2013 Bui Phuc Duyet
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "../ffplay.h"

#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avfilter.h"
#include "drawutils.h"
#include "formats.h"
#include "internal.h"
#include "video.h"

#define R 0
#define G 1
#define B 2
#define A 3

unsigned char eecm[COLOR_SPACE_SIZE][3];

typedef struct {
    const AVClass *class;
    uint8_t rgba_map[4];
    int step;
} EECMContext;

#define OFFSET(x) offsetof(EECMContext, x)

static int query_formats(AVFilterContext *ctx)
{
    static const enum AVPixelFormat pix_fmts[] = {
        AV_PIX_FMT_RGB24, AV_PIX_FMT_BGR24,
        AV_PIX_FMT_RGBA,  AV_PIX_FMT_BGRA,
        AV_PIX_FMT_ABGR,  AV_PIX_FMT_ARGB,
        AV_PIX_FMT_0BGR,  AV_PIX_FMT_0RGB,
        AV_PIX_FMT_RGB0,  AV_PIX_FMT_BGR0,
        AV_PIX_FMT_NONE
    };

    ff_set_common_formats(ctx, ff_make_format_list(pix_fmts));
    return 0;
}


static int config_output(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    EECMContext *cb = ctx->priv;
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(outlink->format);

    ff_fill_rgba_map(cb->rgba_map, outlink->format);
    cb->step = av_get_padded_bits_per_pixel(desc) >> 3;

    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    AVFilterContext *ctx = inlink->dst;
    EECMContext *eecmctx = ctx->priv;
    AVFilterLink *outlink = ctx->outputs[0];
    const uint8_t roffset = eecmctx->rgba_map[R];
    const uint8_t goffset = eecmctx->rgba_map[G];
    const uint8_t boffset = eecmctx->rgba_map[B];
    const uint8_t aoffset = eecmctx->rgba_map[A];
    const int step = eecmctx->step;
    const uint8_t *srcrow = in->data[0];
    uint8_t *dstrow;
    AVFrame *out;
    int i, j;
    int colorIdx;

    if (av_frame_is_writable(in)) {
        out = in;
    } else {
        out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
        if (!out) {
            av_frame_free(&in);
            return AVERROR(ENOMEM);
        }
        av_frame_copy_props(out, in);
    }

    dstrow = out->data[0];
    for (i = 0; i < outlink->h; i++) {
        const uint8_t *src = srcrow;
        uint8_t *dst = dstrow;

        for (j = 0; j < outlink->w * step; j += step) {
        	colorIdx = dst[j + roffset] * 256 * 256 + dst[j + goffset] * 256 + dst[j + boffset];
        	//av_log(NULL, AV_LOG_INFO, "%d %d %d |", dst[j + roffset], dst[j + goffset], dst[j + boffset]);
            dst[j + roffset] = eecm[colorIdx][0];
            dst[j + goffset] = eecm[colorIdx][1];
            dst[j + boffset] = eecm[colorIdx][2];
            //av_log(NULL, AV_LOG_INFO, "%d %d %d \n", dst[j + roffset], dst[j + goffset], dst[j + boffset]);
            if (in != out && step == 4)
                dst[j + aoffset] = src[j + aoffset];
        }

        srcrow += in->linesize[0];
        dstrow += out->linesize[0];
    }

    if (in != out)
        av_frame_free(&in);
    return ff_filter_frame(ctx->outputs[0], out);
}

static const AVFilterPad eecm_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
    },
    { NULL }
};

static const AVFilterPad eecm_outputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .config_props = config_output,
    },
    { NULL }
};

AVFilter avfilter_vf_eecm = {
    .name          = "eecm",
    .description   = NULL_IF_CONFIG_SMALL("Mapping energy-efficient colors"),
    .priv_size     = sizeof(EECMContext),
    .query_formats = query_formats,
    .inputs        = eecm_inputs,
    .outputs       = eecm_outputs,
    .flags         = AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC,
};
