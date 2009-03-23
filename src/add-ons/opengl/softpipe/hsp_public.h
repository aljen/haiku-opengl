/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#ifndef __HSP_PUBLIC_H__
#define __HSP_PUBLIC_H__

#include <support/SupportDefs.h>
#include "hsp_bitmap_wrapper.h"

typedef void (*hsp_proc)();

bool
hsp_copy_context(uint64 srcCtxId, uint64 dstCtxId, uint mask);

uint64
hsp_create_layer_context(Bitmap *bitmap, int layerPlane);

bool
hsp_delete_context(uint64 ctxId);

bool
hsp_release_context(uint64 ctxId);

Bitmap*
hsp_get_current_bitmap(void);

uint64
hsp_get_current_context(void);

bool
hsp_make_current(Bitmap *bitmap, uint64 ctxId);

bool
hsp_swap_buffers(Bitmap *bitmap);

hsp_proc
hsp_get_proc_address(const char *procname);

#endif /* __HSP_PUBLIC_H__ */

