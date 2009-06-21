/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#include "glapi/glthread.h"
#include "util/u_debug.h"
#include "pipe/p_screen.h"
#include "hsp_bitmap_wrapper.h"
#include "hsp_device.h"
#include "hsp_winsys.h"
#include "hsp_public.h"
#include "haiku_softpipe_winsys.h"

//#define TRACE_HSP_DEVICE
#ifdef TRACE_HSP_DEVICE
#	define TRACE(x...)	fprintf(stderr, x);
#else
#	define TRACE(x...)
#endif

struct hsp_device *hsp_dev = NULL;

static const struct hsp_winsys hgl_winsys = {
	&haiku_softpipe_screen_create,
	&haiku_softpipe_context_create,
	&haiku_softpipe_flush_frontbuffer
};

static void
st_flush_frontbuffer(struct pipe_screen *screen, struct pipe_surface *surface,
	void *context_private)
{
	TRACE("%s(screen: %p, surface: %p, context: %p)\n", __FUNCTION__,
		screen, surface, context_private);
	const struct hsp_winsys *hsp_winsys = hsp_dev->hsp_winsys;
	Bitmap *bitmap = (Bitmap*)context_private;

	hsp_winsys->flush_frontbuffer(screen, surface, bitmap);
}

bool
hsp_init(ulong options)
{
	static struct hsp_device hsp_dev_storage;

	TRACE("%s\n", __FUNCTION__);

	assert(!hsp_dev);

	hsp_dev = &hsp_dev_storage;
	memset(hsp_dev, 0, sizeof(*hsp_dev));

#ifdef DEBUG
	hsp_dev->memdb_no = debug_memory_begin();
#endif

	hsp_dev->hsp_winsys = &hgl_winsys;
	hsp_dev->options = options;

	hsp_dev->screen = hgl_winsys.create_screen();
	if (!hsp_dev->screen) {
		TRACE("%s> can't create screen!\n", __FUNCTION__);
		goto error1;
	}

	hsp_dev->screen->flush_frontbuffer = st_flush_frontbuffer;
	TRACE("%s> st_flush_frontbuffer\n", __FUNCTION__);

	pipe_mutex_init(hsp_dev->mutex);

	return true;

error1:
	hsp_dev = NULL;
	return false;
}


void
hsp_cleanup(void)
{
	uint64 i;

	TRACE("%s\n", __FUNCTION__);

	if (!hsp_dev)
		return;

	pipe_mutex_lock(hsp_dev->mutex);
	{
		for (i = 0; i < HSP_CONTEXT_MAX; i++) {
			if (hsp_dev->ctx_array[i].ctx)
				hsp_delete_context(i + 1);
		}
	}
	pipe_mutex_unlock(hsp_dev->mutex);

	pipe_mutex_destroy(hsp_dev->mutex);

	hsp_dev->screen->destroy(hsp_dev->screen);

#ifdef DEBUG
	debug_memory_end(hsp_dev->memdbg_no);
#endif
	hsp_dev = NULL;
}

struct hsp_context*
hsp_lookup_context(uint64 ctxId)
{
	TRACE("%s(ctxId: %d)\n", __FUNCTION__, ctxId);
	if (ctxId == 0 || ctxId >= HSP_CONTEXT_MAX)
		return NULL;

	if (!hsp_dev)
		return NULL;
	struct hsp_context *ctx = hsp_dev->ctx_array[ctxId - 1].ctx;
	TRACE("%s> ctx: %p\n", __FUNCTION__, ctx);
	return ctx;
}

