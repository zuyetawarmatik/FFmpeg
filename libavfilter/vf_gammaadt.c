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

typedef struct {
    const AVClass *class;
    uint8_t rgba_map[4];
    int step;
} GammaAdaptiveContext;

#define OFFSET(x) offsetof(GammaAdaptiveContext, x)

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
    GammaAdaptiveContext *cb = ctx->priv;
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(outlink->format);

    ff_fill_rgba_map(cb->rgba_map, outlink->format);
    cb->step = av_get_padded_bits_per_pixel(desc) >> 3;

    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    AVFilterContext *ctx = inlink->dst;
    GammaAdaptiveContext *gadtctx = ctx->priv;
    AVFilterLink *outlink = ctx->outputs[0];
    const uint8_t roffset = gadtctx->rgba_map[R];
    const uint8_t goffset = gadtctx->rgba_map[G];
    const uint8_t boffset = gadtctx->rgba_map[B];
    const uint8_t aoffset = gadtctx->rgba_map[A];
    const int step = gadtctx->step;
    const uint8_t *srcrow = in->data[0];
    uint8_t *dstrow;
    AVFrame *out;
    int i, j;

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

    double brightness = 0;
    int numpix = 0;
    for (i = 0; i < outlink->h; i += 20) {
		uint8_t *dst = dstrow;

		for (j = 0; j < outlink->w * step; j += step * 20) {
			brightness += sqrt(0.241 * dst[j + roffset] * dst[j + roffset] + 0.691 * dst[j + goffset] * dst[j + goffset] + 0.068 * dst[j + boffset] * dst[j + boffset]);
			numpix++;
		}
	}

    brightness /= numpix;

    float gamma = 0.072 * brightness * 13.0 / 255.0 + 1;
    unsigned char gammaLUT[256];
    for (i = 0; i < 256; i++)
        gammaLUT[i] = (unsigned char) (255 * (pow((double) i / 255.0, gamma)));

    for (i = 0; i < outlink->h; i++) {
        const uint8_t *src = srcrow;
        uint8_t *dst = dstrow;

        for (j = 0; j < outlink->w * step; j += step) {
        	dst[j + roffset] = gammaLUT[dst[j + roffset]];
     	    dst[j + goffset] = gammaLUT[dst[j + goffset]];
        	dst[j + boffset] = gammaLUT[dst[j + boffset]];

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

static const AVFilterPad gammaadt_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
    },
    { NULL }
};

static const AVFilterPad gammaadt_outputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .config_props = config_output,
    },
    { NULL }
};

AVFilter avfilter_vf_gammaadt = {
    .name          = "gammaadt",
    .description   = NULL_IF_CONFIG_SMALL("Gamma Adaptive"),
    .priv_size     = sizeof(GammaAdaptiveContext),
    .query_formats = query_formats,
    .inputs        = gammaadt_inputs,
    .outputs       = gammaadt_outputs,
    .flags         = AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC,
};
