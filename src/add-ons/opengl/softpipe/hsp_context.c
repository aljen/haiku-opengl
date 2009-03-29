/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#include <support/ByteOrder.h>
#include <opengl/GLView.h>
#include "main/mtypes.h"
#include "main/context.h"
#include "pipe/p_compiler.h"
#include "pipe/p_context.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_public.h"
#include "hsp_bitmap_wrapper.h"
#include "hsp_device.h"
#include "hsp_winsys.h"
#include "hsp_framebuffer.h"
#include "hsp_public.h"
#include "hsp_context.h"

//#define TRACE_HSP_CONTEXT
#ifdef TRACE_HSP_CONTEXT
#	define TRACE(x...)	fprintf(stderr, x);
#else
#	define TRACE(x...)
#endif

static Bitmap *current_bitmap = NULL;
static uint64 current_ctx_id = 0;

bool
hsp_copy_context(uint64 srcCtxId, uint64 dstCtxId, uint mask)
{
	TRACE("%s(src: %d dst: %d mask: %d)\n", __FUNCTION__,
		srcCtxId, dstCtxId, mask);
	struct hsp_context *src;
	struct hsp_context *dst;
	bool ret = false;

	pipe_mutex_lock(hsp_dev->mutex);

	src = hsp_lookup_context(srcCtxId);
	dst = hsp_lookup_context(dstCtxId);

	if (src && dst) {
		#warning Implement copying context!
		(void)src;
		(void)dst;
		(void)mask;
	}

	pipe_mutex_unlock(hsp_dev->mutex);
	return ret;
}

uint64
hsp_create_layer_context(Bitmap *bitmap, int layerPlane)
{
	TRACE("%s(bitmap: %p layerPlane: %d)\n", __FUNCTION__,
		bitmap, layerPlane);
	time_t total_beg, total_end;
	time_t beg, end;
	total_beg = time(NULL);
	struct hsp_context *ctx = NULL;
	GLvisual *visual = NULL;
	struct pipe_context *pipe = NULL;
	uint ctxId = 0;

	if (!hsp_dev) {
		TRACE("%s> there is no hsp_dev!\n", __FUNCTION__);
		return 0;
	}

	if (layerPlane != 0) {
		TRACE("%s> layerPlane != 0\n", __FUNCTION__);
		return 0;
	}

	ctx = CALLOC_STRUCT(hsp_context);
	if (!ctx) {
		TRACE("%s> can't alloc hsp_context!\n", __FUNCTION__);
		return 0;
	}

	ctx->bitmap = bitmap;
	ctx->colorSpace = get_bitmap_color_space(bitmap);
	ctx->draw = NULL;
	ctx->read = NULL;

	ulong options = hsp_dev->options;

	const GLboolean rgbFlag		= ((options & BGL_INDEX) == 0);
	const GLboolean alphaFlag	= ((options & BGL_ALPHA) == BGL_ALPHA);
	const GLboolean dblFlag		= ((options & BGL_DOUBLE) == BGL_DOUBLE);
	const GLboolean stereoFlag	= false;
	const GLint depth		= (options & BGL_DEPTH) ? 24 : 0;
	const GLint stencil		= (options & BGL_STENCIL) ? 8 : 0;
	const GLint accum		= 0;					// (options & BGL_ACCUM) ? 16 : 0;
	const GLint index		= 0;					// (options & BGL_INDEX) ? 32 : 0;
	const GLint red			= rgbFlag ? 8 : 0;
	const GLint green		= rgbFlag ? 8 : 0;
	const GLint blue		= rgbFlag ? 8 : 0;
	const GLint alpha		= alphaFlag ? 8 : 0;

	TRACE("rgb         :\t%d\n", (bool)rgbFlag);
	TRACE("alpha       :\t%d\n", (bool)alphaFlag);
	TRACE("dbl         :\t%d\n", (bool)dblFlag);
	TRACE("stereo      :\t%d\n", (bool)stereoFlag);
	TRACE("depth       :\t%d\n", depth);
	TRACE("stencil     :\t%d\n", stencil);
	TRACE("accum       :\t%d\n", accum);
	TRACE("index       :\t%d\n", index);
	TRACE("red         :\t%d\n", red);
	TRACE("green       :\t%d\n", green);
	TRACE("blue        :\t%d\n", blue);
	TRACE("alpha       :\t%d\n", alpha);

	visual = _mesa_create_visual(
		rgbFlag, dblFlag, stereoFlag, red, green, blue, alpha, index, depth,
		stencil, accum, accum, accum, alpha ? accum : 0, 1);

	TRACE("depthBits   :\t%d\n", visual->depthBits);
	TRACE("stencilBits :\t%d\n", visual->stencilBits);

	if (!visual) {
		TRACE("%s> can't create mesa visual!\n", __FUNCTION__);
		goto fail;
	}

	pipe = hsp_dev->hsp_winsys->create_context(hsp_dev->screen);

	if (!pipe) {
		TRACE("%s> can't create pipe!\n", __FUNCTION__);
		goto fail;
	}

	assert(!pipe->priv);
	pipe->priv = bitmap;

	ctx->st = st_create_context(pipe, visual, NULL);
	if (!ctx->st) {
		TRACE("%s> can't create mesa statetracker context!\n",
			__FUNCTION__);
		goto fail;
	}

	ctx->st->ctx->DriverCtx = ctx;

	pipe_mutex_lock(hsp_dev->mutex);
	{
		uint64 i;
		for (i = 0; i < HSP_CONTEXT_MAX; i++) {
			if (hsp_dev->ctx_array[i].ctx == NULL) {
				hsp_dev->ctx_array[i].ctx = ctx;
				ctxId = i + 1;
				break;
			}
		}
	}
	pipe_mutex_unlock(hsp_dev->mutex);

	if (ctxId != 0)
		return ctxId;

fail:
	if (visual)
		_mesa_destroy_visual(visual);

	if (pipe)
		pipe->destroy(pipe);

	FREE(ctx);
	return 0;
}

bool
hsp_delete_context(uint64 ctxId)
{
	TRACE("%s(ctxId: %d)\n", __FUNCTION__, ctxId);
	struct hsp_context *ctx = NULL;
	bool ret = false;

	if (!hsp_dev) {
		TRACE("%s> there's no hsp_dev so nothing to do\n",
			__FUNCTION__);
		return false;
	}

	pipe_mutex_lock(hsp_dev->mutex);

	ctx = hsp_lookup_context(ctxId);
	if (ctx) {
		TRACE("%s> found context: %p\n", __FUNCTION__, ctx);
		GLcontext *glctx = ctx->st->ctx;
		GET_CURRENT_CONTEXT(glcurctx);

		if (glcurctx == glctx)
			st_make_current(NULL, NULL, NULL);

		if (ctx->draw) {
			TRACE("%s> found context draw framebuffer, destroying: %p\n",
				__FUNCTION__, ctx->draw);
			framebuffer_destroy(ctx->draw);
		}
		if (ctx->read) {
			TRACE("%s> found context read framebuffer, destroying: %p\n",
				__FUNCTION__, ctx->read);
			framebuffer_destroy(ctx->read);
		}

		#warning TODO: destroy ctx->bitmap here ?
		
		st_destroy_context(ctx->st);

		FREE(ctx);

		hsp_dev->ctx_array[ctxId - 1].ctx = NULL;
		ret = true;
	}

	pipe_mutex_unlock(hsp_dev->mutex);
	return ret;
}

bool
hsp_release_context(uint64 ctxId)
{
	TRACE("%s(ctxId: %d)\n", __FUNCTION__, ctxId);
	bool ret = false;

	if (!hsp_dev) {
		TRACE("%s> there's no hsp_dev so nothing to do\n",
			__FUNCTION__);
		return ret;
	}

	pipe_mutex_lock(hsp_dev->mutex);
	{
		struct hsp_context *ctx = NULL;

		ctx = hsp_lookup_context(ctxId);
		if (!ctx)
			goto done;
		if (!hsp_make_current(NULL, 0))
			goto done;
		ret = true;
	}
done:
	pipe_mutex_unlock(hsp_dev->mutex);
	return ret;
}

uint64
hsp_get_current_context(void)
{
	TRACE("%s> current context id: %d\n", __FUNCTION__, current_ctx_id);
	return current_ctx_id;
}

bool
hsp_make_current(Bitmap *bitmap, uint64 ctxId)
{
	TRACE("%s(bitmap: %p ctxId: %d)\n", __FUNCTION__, bitmap, ctxId);
	struct hsp_context *ctx = NULL;
	GET_CURRENT_CONTEXT(glcurctx);
	GLuint width = 0;
	GLuint height = 0;
	struct hsp_context *curctx;

	if (!hsp_dev) {
		TRACE("%s> there's no hsp_dev so nothing to do\n",
			__FUNCTION__);
		return false;
	}

	pipe_mutex_lock(hsp_dev->mutex);
	ctx = hsp_lookup_context(ctxId);
	pipe_mutex_unlock(hsp_dev->mutex);

	if (ctx == NULL) {
		TRACE("%s> context not found\n", __FUNCTION__);
		return false;
	}

	current_bitmap = bitmap;
	current_ctx_id = ctxId;

	if (glcurctx != NULL) {
		curctx = (struct hsp_context*) glcurctx->DriverCtx;

		if (curctx != ctx)
			st_flush(glcurctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
	}

	if (!bitmap || ctxId == 0) {
		st_make_current(NULL, NULL, NULL);
		return true;
	}

	if (glcurctx != NULL) {
		struct hsp_context *curctx = (struct hsp_context*) glcurctx->DriverCtx;
		if (curctx != NULL && curctx == ctx && ctx->bitmap == bitmap)
			return true;
	}

	if (bitmap != NULL) {
		get_bitmap_size(bitmap, &width, &height);
		TRACE("%s> found bitmap: %p with size: %dx%d\n", __FUNCTION__,
			bitmap, width, height);
	}

	if (ctx != NULL && bitmap != NULL ) {
		GLvisual *visual = &ctx->st->ctx->Visual;
		if (ctx->draw == NULL) {
			ctx->draw = framebuffer_create(bitmap, visual, width /*+ 1*/, height /*+ 1*/);
		}
		if ((hsp_dev->options & BGL_DOUBLE) == BGL_DOUBLE && ctx->read == NULL) {
			ctx->read = framebuffer_create(bitmap, visual, width /*+ 1*/, height /*+ 1*/);
		}
	}

	if (ctx) {
		if (ctx->draw && ctx->read == NULL) {
			st_make_current(ctx->st, ctx->draw->stfb, ctx->draw->stfb);
			framebuffer_resize(ctx->draw, width /*+ 1*/, height /*+ 1*/);
		} else if (ctx && ctx->draw && ctx->read) {
			st_make_current(ctx->st, ctx->draw->stfb, ctx->read->stfb);
			framebuffer_resize(ctx->draw, width /*+ 1*/, height /*+ 1*/);
			framebuffer_resize(ctx->read, width /*+ 1*/, height /*+ 1*/);
		}
		ctx->bitmap = bitmap;
		ctx->st->pipe->priv = bitmap;
	} else {
		st_make_current(NULL, NULL, NULL);
	}

	return true;
}

