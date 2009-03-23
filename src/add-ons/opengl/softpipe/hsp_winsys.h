/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#ifndef __HSP_WINSYS_H__
#define __HSP_WINSYS_H__

#include "pipe/internal/p_winsys_screen.h"
#include "pipe/p_compiler.h"
#include "hsp_bitmap_wrapper.h"

struct pipe_context;
struct pipe_screen;
struct pipe_surface;

struct hsp_winsys
{
	struct pipe_screen*
		(*create_screen)(void);
	struct pipe_context*
		(*create_context)(struct pipe_screen *screen);
	void
		(*flush_frontbuffer)(struct pipe_screen *screen,
			struct pipe_surface *surface, Bitmap *bitmap);
};

bool
hsp_init(ulong options);

void
hsp_cleanup(void);

#endif /* __HSP_WINSYS_H__ */

