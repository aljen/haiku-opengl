/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#ifndef __HSP_CONTEXT_H__
#define __HSP_CONTEXT_H__

#include "hsp_bitmap_wrapper.h"

struct st_context;

struct hsp_context
{
	struct st_context *st;
	Bitmap *bitmap;
	color_space colorSpace;
	struct hsp_framebuffer *draw;
	struct hsp_framebuffer *read;
};

#endif /* __HSP_CONTEXT_H__ */

