/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#include "glapi/glapi.h"
#include "hsp_public.h"

hsp_proc
hsp_get_proc_address(const char *procname)
{
	hsp_proc p = (hsp_proc)_glapi_get_proc_address((const char*)procname);
	if (p)
		return p;
	return NULL;
}

