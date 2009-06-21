/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#ifndef __HSP_DEVICE_H__
#define __HSP_DEVICE_H__

#include "hsp_public.h"
#include "pipe/p_compiler.h"
#include "pipe/p_thread.h"
struct pipe_screen;

#define HSP_CONTEXT_MAX	32

struct hsp_device
{
	const struct hsp_winsys *hsp_winsys;
	struct pipe_screen *screen;
	ulong options;
	pipe_mutex mutex;
	struct {
		struct hsp_context *ctx;
	} ctx_array[HSP_CONTEXT_MAX];
#ifdef DEBUG
	unsigned long memdb_no;
	bool trace_running;
#endif
};

struct hsp_context*
hsp_lookup_context(uint64 ctxId);

extern struct hsp_device *hsp_dev;

#endif /* __HSP_DEVICE_H__ */

