/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#include "haiku_softpipe_winsys.h"
#include "hsp_device.h"

//#define TRACE_HAIKU_SOFTPIPE
#if defined(TRACE_HAIKU_SOFTPIPE)
#	define TRACE(x...)	fprintf(stderr, x);
#else
#	define TRACE(x...)
#endif

static INLINE struct haiku_softpipe_buffer*
haiku_softpipe_buffer(struct pipe_buffer *buf)
{
	return (struct haiku_softpipe_buffer*)buf;
}

//static
void*
haiku_softpipe_buffer_map(struct pipe_winsys *winsys, struct pipe_buffer *buf,
	unsigned flags)
{
	TRACE("%s(winsys: %p buf: %p flags: %d)\n", __FUNCTION__,
		winsys, buf, flags);
	struct haiku_softpipe_buffer *haiku_softpipe_buf = haiku_softpipe_buffer(buf);
	haiku_softpipe_buf->mapped = haiku_softpipe_buf->data;
	return haiku_softpipe_buf->mapped;
}

//static
void
haiku_softpipe_buffer_unmap(struct pipe_winsys *winsys, struct pipe_buffer *buf)
{
	TRACE("%s(winsys: %p buf: %p)\n", __FUNCTION__, winsys, buf);
	struct haiku_softpipe_buffer *haiku_softpipe_buf = haiku_softpipe_buffer(buf);
	haiku_softpipe_buf->mapped = NULL;
}

//static
void
haiku_softpipe_buffer_destroy(struct pipe_buffer *buf)
{
	TRACE("%s(buf: %p)\n", __FUNCTION__, buf);
	struct haiku_softpipe_buffer *oldBuf = haiku_softpipe_buffer(buf);

	if (oldBuf->data) {
		if (!oldBuf->user_buffer)
			align_free(oldBuf->data);
		oldBuf->data = NULL;
	}
	FREE(oldBuf);
}

//static
const char*
haiku_softpipe_get_name(struct pipe_winsys *winsys)
{
	TRACE("%s(winsys: %p)\n", __FUNCTION__, winsys);
	return "Haiku SoftPipe";
}

//static
struct pipe_buffer*
haiku_softpipe_buffer_create(struct pipe_winsys *winsys, unsigned alignment,
	unsigned usage, unsigned size)
{
	TRACE("%s(winsys: %p alignment: %d usage: %d size: %d)\n",
		__FUNCTION__, winsys, alignment, usage, size);
	struct haiku_softpipe_buffer *buffer = CALLOC_STRUCT(haiku_softpipe_buffer);

	pipe_reference_init(&buffer->base.reference, 1);
	buffer->base.alignment = alignment;
	buffer->base.usage = usage;
	buffer->base.size = size;

	buffer->data = align_malloc(size, alignment);

	return &buffer->base;
}

//static
struct pipe_buffer*
haiku_softpipe_user_buffer_create(struct pipe_winsys *winsys, void *ptr,
	unsigned bytes)
{
	TRACE("%s(winsys: %p ptr: %p bytes: %d)\n", __FUNCTION__,
		winsys, ptr, bytes);
	struct haiku_softpipe_buffer *buffer;

	buffer = CALLOC_STRUCT(haiku_softpipe_buffer);

	if (!buffer)
		return NULL;

	pipe_reference_init(&buffer->base.reference, 1);
	buffer->base.size = bytes;
	buffer->user_buffer = true;
	buffer->data = ptr;

	return &buffer->base;
}

static INLINE unsigned
round_up(unsigned n, unsigned multiple)
{
	return (n + multiple - 1) & ~(multiple - 1);
}

//static
struct pipe_buffer*
haiku_softpipe_surface_buffer_create(struct pipe_winsys *winsys,
	unsigned width, unsigned height, enum pipe_format format,
	unsigned usage, unsigned *stride)
{
	TRACE("%s(winsys: %p width: %d height: %d format: %d usage: %d "
		"stride: %p)\n", __FUNCTION__, winsys, width, height, format,
		usage, stride);
	const unsigned alignment = 32;
	struct pipe_format_block block;
	unsigned nblocksx, nblocksy;

	pf_get_block(format, &block);
	nblocksx = pf_get_nblocksx(&block, width);
	nblocksy = pf_get_nblocksy(&block, height);
	*stride = round_up(nblocksx * block.size, alignment);

	return winsys->buffer_create(winsys, alignment, usage, *stride * nblocksy);
}

//static
void
haiku_softpipe_dummy_flush_frontbuffer(struct pipe_winsys *winsys,
	struct pipe_surface *surface, void *context_private)
{
	TRACE("%s(winsys: %p surface: %p context: %p)\n", __FUNCTION__,
		winsys, surface, context_private);
	assert(0);
}

//static
void
haiku_softpipe_fence_reference(struct pipe_winsys *winsys,
	struct pipe_fence_handle **ptr, struct pipe_fence_handle *fence)
{
	TRACE("%s(winsys: %p ptr: %p fence: %p)\n", __FUNCTION__,
		winsys, ptr, fence);
}

//static
int
haiku_softpipe_fence_signalled(struct pipe_winsys *winsys,
	struct pipe_fence_handle *fence, unsigned flag)
{
	TRACE("%s(winsys: %p fence: %p flag: %d)\n", __FUNCTION__,
		winsys, fence, flag);
	return 0;
}

//static
int
haiku_softpipe_fence_finish(struct pipe_winsys *winsys,
	struct pipe_fence_handle *fence, unsigned flag)
{
	TRACE("%s(winsys: %p fence: %p flag: %d)\n", __FUNCTION__,
		winsys, fence, flag);
	return 0;
}

//static
void
haiku_softpipe_destroy(struct pipe_winsys *winsys)
{
	TRACE("%s(winsys: %p)\n", __FUNCTION__, winsys);
	FREE(winsys);
}

//static
struct pipe_screen*
haiku_softpipe_screen_create(void)
{
	TRACE("%s\n", __FUNCTION__);
	static struct pipe_winsys *winsys;
	struct pipe_screen *screen;

	winsys = CALLOC_STRUCT(pipe_winsys);
	
	if (!winsys) {
		TRACE("%s> can't alloc winsys!\n", __FUNCTION__);
		return NULL;
	}

	winsys->destroy = haiku_softpipe_destroy;
	winsys->buffer_create = haiku_softpipe_buffer_create;
	winsys->user_buffer_create = haiku_softpipe_user_buffer_create;
	winsys->buffer_map = haiku_softpipe_buffer_map;
	winsys->buffer_unmap = haiku_softpipe_buffer_unmap;
	winsys->buffer_destroy = haiku_softpipe_buffer_destroy;
	winsys->surface_buffer_create = haiku_softpipe_surface_buffer_create;
	winsys->fence_reference = haiku_softpipe_fence_reference;
	winsys->fence_signalled = haiku_softpipe_fence_signalled;
	winsys->fence_finish = haiku_softpipe_fence_finish;
	winsys->flush_frontbuffer = haiku_softpipe_dummy_flush_frontbuffer;
	winsys->get_name = haiku_softpipe_get_name;

	TRACE("%s> @softpipe_create_screen(winsys: %p)\n", __FUNCTION__,
		winsys);

	screen = softpipe_create_screen(winsys);

	if (!screen) {
		TRACE("%s> can't create screen!\n", __FUNCTION__);
		haiku_softpipe_destroy(winsys);
	}

	return screen;
}

//static
struct pipe_context*
haiku_softpipe_context_create(struct pipe_screen *screen)
{
	TRACE("haiku_softpipe_context_create> screen = %p\n", screen);
	struct pipe_context *pipe = softpipe_create(screen);
	return pipe;
}

//static
void
haiku_softpipe_flush_frontbuffer(struct pipe_screen *screen,
	struct pipe_surface *surface, Bitmap* bitmap)
{
	TRACE("haiku_softpipe_flush_frontbuffer> screen = %p, "
		"surface = %p, bitmap = %p\n", screen, surface, bitmap);
	struct softpipe_texture *texture;
	struct haiku_softpipe_buffer *buffer;

	texture = softpipe_texture(surface->texture);

	buffer = haiku_softpipe_buffer(texture->buffer);

	int32 bitsLength = get_bitmap_bits_length(bitmap);
	copy_bitmap_bits(bitmap, buffer->data, bitsLength);
}
