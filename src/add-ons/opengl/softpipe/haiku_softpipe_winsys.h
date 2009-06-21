/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#ifndef __HAIKU_SOFTPIPE_H__
#define __HAIKU_SOFTPIPE_H__

#include <stdio.h>
#include "pipe/internal/p_winsys_screen.h"
#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "sp_winsys.h"
#include "sp_texture.h"
#include "hsp_bitmap_wrapper.h"
#include "hsp_winsys.h"

struct haiku_softpipe_buffer {
	struct pipe_buffer base;
	bool user_buffer;
	void *data;
	void *mapped;
};

static INLINE struct haiku_softpipe_buffer*
haiku_softpipe_buffer(struct pipe_buffer *buf);

//static
void*
haiku_softpipe_buffer_map(struct pipe_winsys *winsys, struct pipe_buffer *buf,
	unsigned flags);

//static
void
haiku_softpipe_buffer_unmap(struct pipe_winsys *winsys, struct pipe_buffer *buf);

//static
void
haiku_softpipe_buffer_destroy(struct pipe_buffer *buf);

//static
const char*
haiku_softpipe_get_name(struct pipe_winsys *winsys);

//static
struct pipe_buffer*
haiku_softpipe_buffer_create(struct pipe_winsys *winsys, unsigned alignment,
	unsigned usage, unsigned size);

//static
struct pipe_buffer*
haiku_softpipe_user_buffer_create(struct pipe_winsys *winsys, void *ptr,
	unsigned bytes);

//static
struct pipe_buffer*
haiku_softpipe_surface_buffer_create(struct pipe_winsys *winsys,
	unsigned width, unsigned height, enum pipe_format format,
	unsigned usage, unsigned *stride);

//static
void
haiku_softpipe_dummy_flush_frontbuffer(struct pipe_winsys *winsys,
	struct pipe_surface *surface, void *context_private);

//static
void
haiku_softpipe_fence_reference(struct pipe_winsys *winsys,
	struct pipe_fence_handle **ptr, struct pipe_fence_handle *fence);

//static
int
haiku_softpipe_fence_signalled(struct pipe_winsys *winsys,
	struct pipe_fence_handle *fence, unsigned flag);

//static
int
haiku_softpipe_fence_finish(struct pipe_winsys *winsys,
	struct pipe_fence_handle *fence, unsigned flag);

//static
void
haiku_softpipe_destroy(struct pipe_winsys *winsys);

//static
struct pipe_screen*
haiku_softpipe_screen_create(void);

//static
struct pipe_context*
haiku_softpipe_context_create(struct pipe_screen *screen);

//static
void
haiku_softpipe_flush_frontbuffer(struct pipe_screen *screen,
	struct pipe_surface *surface, Bitmap* bitmap);

#endif /* __HAIKU_SOFTPIPE_WINSYS_H__ */

