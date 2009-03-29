/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#include "main/context.h"
#include "pipe/p_format.h"
#include "pipe/p_screen.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_public.h"
#include "hsp_bitmap_wrapper.h"
#include "hsp_framebuffer.h"
#include "hsp_context.h"
#include "hsp_device.h"
#include "hsp_public.h"
#include "hsp_winsys.h"

//#define TRACE_FRAMEBUFFER
#ifdef TRACE_FRAMEBUFFER
#	define TRACE(x...)	fprintf(stderr, x);
#else
#	define TRACE(x...)
#endif

void
framebuffer_resize(struct hsp_framebuffer *fb, GLuint width, GLuint height)
{
	TRACE("%s(fb: %p width: %d height: %d)\n", __FUNCTION__, fb, width,
		height);
	TRACE("%s IMPLEMENT!\n", __FUNCTION__);
	st_resize_framebuffer(fb->stfb, width, height);
}

static struct hsp_framebuffer *fb_head = NULL;

static INLINE bool
hsp_is_supported_depth_stencil(enum pipe_format format)
{
	TRACE("%s(format: %d)\n", __FUNCTION__, format);
	struct pipe_screen *screen = hsp_dev->screen;
	return screen->is_format_supported(screen, format, PIPE_TEXTURE_2D,
		PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);
}

struct hsp_framebuffer*
framebuffer_create(Bitmap *bitmap, GLvisual *visual, GLuint width, GLuint height)
{
	TRACE("%s(bitmap: %p visual: %p width: %d height: %d)\n", __FUNCTION__,
		bitmap, visual, width, height);
	struct hsp_framebuffer *fb = NULL;
	enum pipe_format colorFormat, depthFormat, stencilFormat;

	fb = CALLOC_STRUCT(hsp_framebuffer);

	if (!fb) {
		TRACE("%s> can't alloc framebuffer!\n", __FUNCTION__);
		return NULL;
	}

	colorFormat = PIPE_FORMAT_A8R8G8B8_UNORM;

	if (visual->depthBits == 32)
		depthFormat = PIPE_FORMAT_Z32_UNORM;
	else if (visual->depthBits == 24)
		depthFormat = PIPE_FORMAT_S8Z24_UNORM;
	else if (visual->depthBits == 16)
		depthFormat = PIPE_FORMAT_Z16_UNORM;
	else
		depthFormat = PIPE_FORMAT_NONE;

	if (visual->stencilBits == 8) {
		if (depthFormat == PIPE_FORMAT_S8Z24_UNORM)
			stencilFormat = depthFormat;
		else
			stencilFormat = PIPE_FORMAT_S8_UNORM;
	} else
		stencilFormat = PIPE_FORMAT_NONE;

	colorFormat = PIPE_FORMAT_A8R8G8B8_UNORM;

        TRACE("\tredBits     = %d\n", visual->redBits);
	TRACE("\tgreenBits   = %d\n", visual->greenBits);
	TRACE("\tblueBits    = %d\n", visual->blueBits);
	TRACE("\talphaBits   = %d\n", visual->alphaBits);
	TRACE("\tdepthBits   = %d\n", visual->depthBits);
	TRACE("\tstencilBits = %d\n", visual->stencilBits);

	fb->stfb = st_create_framebuffer(visual, colorFormat, depthFormat,
			stencilFormat, width, height, (void*)fb);
	if (!fb->stfb) {
		TRACE("%s> can't create mesa statetracker framebuffer!\n",
			__FUNCTION__);
	}
	fb->bitmap = bitmap;
	fb->next = fb_head;
	fb_head = fb;
	return fb;
}

void
framebuffer_destroy(struct hsp_framebuffer *fb)
{
	TRACE("%s(fb: %p)\n", __FUNCTION__, fb);
	struct hsp_framebuffer **link = &fb_head;
	struct hsp_framebuffer *pfb = fb_head;

	while (pfb != NULL) {
		if (pfb == fb) {
			*link = fb->next;
			FREE(fb);
			return;
		}
		link = &pfb->next;
		pfb = pfb->next;
	}
}

bool
hsp_swap_buffers(uint64 ctxId)
{
	TRACE("%s(ctxId: %p)\n", __FUNCTION__, ctxId);
	struct hsp_context *ctx = NULL;
	struct hsp_framebuffer *fb = NULL;
	struct pipe_surface *surface;

	pipe_mutex_lock(hsp_dev->mutex);
	ctx = hsp_lookup_context(ctxId);
	pipe_mutex_unlock(hsp_dev->mutex);

	if (ctx == NULL) {
		TRACE("%s> context not found\n", __FUNCTION__);
		return false;
	}
	
	fb = ctx->draw;
	
	st_notify_swapbuffers(fb->stfb);

	st_get_framebuffer_surface(fb->stfb, ST_SURFACE_BACK_LEFT, &surface);

	hsp_dev->hsp_winsys->flush_frontbuffer(hsp_dev->screen, surface, ctx->bitmap);
	
	return true;
}

