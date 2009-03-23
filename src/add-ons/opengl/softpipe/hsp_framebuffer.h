/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#ifndef __HSP_FRAMEBUFFER_H__
#define __HSP_FRAMEBUFFER_H__

#include <interface/GraphicsDefs.h>
#include "main/mtypes.h"
#include "hsp_bitmap_wrapper.h"
#include "hsp_public.h"

struct hsp_framebuffer
{
	struct st_framebuffer *stfb;
	Bitmap *bitmap;
	int pixelformat;
	color_space colorSpace;
	struct hsp_framebuffer *next;
};

struct hsp_framebuffer*
framebuffer_create(Bitmap *bitmap, GLvisual *visual, GLuint width,
	GLuint height);

void
framebuffer_destroy(struct hsp_framebuffer *fb);

void
framebuffer_resize(struct hsp_framebuffer *fb, GLuint width, GLuint height);

struct hsp_framebuffer*
framebuffer_from_bitmap(Bitmap *bitmap);

#endif /* __HSP_FRAMEBUFFER_H__ */

