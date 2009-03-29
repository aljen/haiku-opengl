/*
 * Copyright 2006-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 *		Philippe Houdoin, philippe.houdoin@free.fr
 *		Artur Wyszynski, harakash@gmail.com
 */
/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 */

#include "SoftPipeRenderer.h"

#include <Autolock.h>
#include <DirectWindowPrivate.h>
#include <GraphicsDefs.h>
#include <Screen.h>
#include <sys/time.h>

#define TRACE_SOFTPIPE
#ifdef TRACE_SOFTPIPE
#	define TRACE(x...)	fprintf(stderr, x);
#	define CALLED()		TRACE("CALLED: %s\n", __FUNCTION__);
#else
#	define TRACE(x...)
#	define CALLED()
#endif

extern "C" {
#include "hsp_public.h"
#include "hsp_winsys.h"
#include "haiku_softpipe_winsys.h"
}

extern const char* color_space_name(color_space space);

extern "C" _EXPORT BGLRenderer*
instantiate_gl_renderer(BGLView *view, ulong opts, BGLDispatcher *dispatcher)
{
	return new SoftPipeRenderer(view, opts, dispatcher);
}


SoftPipeRenderer::SoftPipeRenderer(BGLView *view, ulong options,
		BGLDispatcher* dispatcher)
	: BGLRenderer(view, options, dispatcher),
	fBitmap(NULL),
	fDirectModeEnabled(false),
	fInfo(NULL),
	fInfoLocker("info locker"),
	fColorSpace(B_NO_COLOR_SPACE),
	fOptions(options)
{
	CALLED();
	time_t beg, end;
	beg = time(NULL);
	hsp_init(options);
	end = time(NULL);
	TRACE("hsp_init time: %f.\n", difftime(end, beg));
	BRect b = GLView()->Bounds();
	fColorSpace = B_RGBA32;
	if (fDirectModeEnabled && fInfo != NULL) {
		fColorSpace = BScreen(GLView()->Window()).ColorSpace();
	}
	int32 width = b.IntegerWidth();// + 1;
	int32 height = b.IntegerHeight();// + 1;
	TRACE("ColorSpace:\t%s\n", color_space_name(fColorSpace));
	fBitmap = new BBitmap(BRect(0.f, 0.f, width /*- 1*/, height /*- 1*/), fColorSpace);
	fWidth = width;
	fHeight = height;
	beg = time(NULL);
	fContext = hsp_create_layer_context(fBitmap, 0);
	TRACE("context:\t%d\n", fContext);
	end = time(NULL);
	TRACE("hsp_create_layer_context time: %f.\n", difftime(end, beg));
	if (!hsp_get_current_context())
		LockGL();
}

SoftPipeRenderer::~SoftPipeRenderer()
{
	CALLED();
	if (fContext)
		hsp_delete_context(fContext);
	hsp_cleanup();
	delete fBitmap;
}


void
SoftPipeRenderer::LockGL()
{
//	CALLED();
	BGLRenderer::LockGL();
	hsp_make_current(fBitmap, fContext);

}


void
SoftPipeRenderer::UnlockGL()
{
//	CALLED();
//	hsp_make_current(NULL, fContext);
	if ((fOptions & BGL_DOUBLE) == 0) {
		SwapBuffers();
	}
	hsp_make_current(NULL, fContext);
	BGLRenderer::UnlockGL();
}


void
SoftPipeRenderer::SwapBuffers(bool vsync)
{
//	CALLED();
	hsp_swap_buffers(fContext);
	if (!fDirectModeEnabled || fInfo == NULL) {
		GLView()->LockLooper();
		GLView()->DrawBitmap(fBitmap);
		GLView()->UnlockLooper();
	} else {
		BAutolock lock(fInfoLocker);
		uint8 bytesPerPixel = fInfo->bits_per_pixel / 8;
		uint32 bytesPerRow = fBitmap->BytesPerRow();
		for (uint32 i = 0; i < fInfo->clip_list_count; i++) {
			clipping_rect *clip = &fInfo->clip_list[i];
			int32 height = clip->bottom - clip->top + 1;
			int32 bytesWidth 
				= (clip->right - clip->left + 1) * bytesPerPixel;
			uint8 *p = (uint8 *)fInfo->bits + clip->top
				* fInfo->bytes_per_row + clip->left * bytesPerPixel;
			uint8 *b = (uint8 *)fBitmap->Bits()
				+ (clip->top - fInfo->window_bounds.top) * bytesPerRow
				+ (clip->left - fInfo->window_bounds.left) * bytesPerPixel;

			for (int y = 0; y < height; y++) {
				memcpy(p, b, bytesWidth);
				p += fInfo->bytes_per_row;
				b += bytesPerRow;
			}
		}
	}
}


void
SoftPipeRenderer::Draw(BRect updateRect)
{
	CALLED();
	if (!fDirectModeEnabled || fInfo == NULL)
		GLView()->DrawBitmap(fBitmap, updateRect, updateRect);
}


status_t
SoftPipeRenderer::CopyPixelsOut(BPoint location, BBitmap *bitmap)
{
	CALLED();
	color_space scs = fBitmap->ColorSpace();
	color_space dcs = bitmap->ColorSpace();

	if (scs != dcs && (scs != B_RGBA32 || dcs != B_RGB32)) {
		fprintf(stderr, "%s::CopyPixelsOut(): incompatible color space: %s != %s\n",
			__FUNCTION__, color_space_name(scs), color_space_name(dcs));
		return B_BAD_TYPE;
	}

	BRect sr = fBitmap->Bounds();
	BRect dr = bitmap->Bounds();

	int32 w1 = sr.IntegerWidth();
	int32 h1 = sr.IntegerHeight();
	int32 w2 = dr.IntegerWidth();
	int32 h2 = dr.IntegerHeight();

	sr = sr & dr.OffsetBySelf(location);
	dr = sr.OffsetByCopy(-location.x, -location.y);

	uint8 *ps = (uint8 *) fBitmap->Bits();
	uint8 *pd = (uint8 *) bitmap->Bits();
	uint32 *s, *d;
	uint32 y;
	for (y = (uint32) sr.top; y <= (uint32) sr.bottom; y++) {
		s = (uint32 *)(ps + y * fBitmap->BytesPerRow());
		s += (uint32) sr.left;

		d = (uint32 *)(pd + (y + (uint32)(dr.top - sr.top))
			* bitmap->BytesPerRow());
		d += (uint32) dr.left;
		memcpy(d, s, dr.IntegerWidth() * 4);
	}

	return B_OK;
}


status_t
SoftPipeRenderer::CopyPixelsIn(BBitmap *bitmap, BPoint location)
{
	CALLED();

	color_space scs = bitmap->ColorSpace();
	color_space dcs = fBitmap->ColorSpace();

	if (scs != dcs && (dcs != B_RGBA32 || scs != B_RGB32)) {
		fprintf(stderr, "%s::CopyPixelsIn(): incompatible color space: %s != %s\n",
			__FUNCTION__, color_space_name(scs), color_space_name(dcs));
		return B_BAD_TYPE;
	}

	BRect sr = bitmap->Bounds();
	BRect dr = fBitmap->Bounds();

	sr = sr & dr.OffsetBySelf(location);
	dr = sr.OffsetByCopy(-location.x, -location.y);

	uint8 *ps = (uint8 *) bitmap->Bits();
	uint8 *pd = (uint8 *) fBitmap->Bits();
	uint32 *s, *d;
	uint32 y;

	for (y = (uint32) sr.top; y <= (uint32) sr.bottom; y++) {
		s = (uint32 *)(ps + y * bitmap->BytesPerRow());
		s += (uint32) sr.left;

		d = (uint32 *)(pd + (y + (uint32)(dr.top - sr.top))
			* fBitmap->BytesPerRow());
		d += (uint32) dr.left;

		memcpy(d, s, dr.IntegerWidth() * 4);
	}

	return B_OK;
}


void
SoftPipeRenderer::EnableDirectMode(bool enabled)
{
//	TRACE("SoftPipeRenderer::%s(enabled: %d)\n", __FUNCTION__, enabled);
	fDirectModeEnabled = enabled;
}


void
SoftPipeRenderer::DirectConnected(direct_buffer_info *info)
{
	CALLED();
	BAutolock lock(fInfoLocker);
	if (info) {
		if (!fInfo) {
			fInfo = (direct_buffer_info *)calloc(1, DIRECT_BUFFER_INFO_AREA_SIZE);
		}
		memcpy(fInfo, info, DIRECT_BUFFER_INFO_AREA_SIZE);
	} else if (fInfo) {
		free(fInfo);
		fInfo = NULL;
	}
}


// #pragma mark - static


void
SoftPipeRenderer::Error(GLcontext *ctx)
{
	CALLED();
}


const GLubyte *
SoftPipeRenderer::GetString(GLcontext *ctx, GLenum name)
{
	CALLED();
}


void
SoftPipeRenderer::Viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w,
		GLsizei h)
{
	CALLED();
}


void
SoftPipeRenderer::UpdateState(GLcontext *ctx, GLuint new_state)
{
	CALLED();
}


void
SoftPipeRenderer::ClearIndex(GLcontext *ctx, GLuint index)
{
	CALLED();
}


void
SoftPipeRenderer::ClearColor(GLcontext *ctx, const GLfloat color[4])
{
	CALLED();
}


void
SoftPipeRenderer::Clear(GLcontext *ctx, GLbitfield mask)
{
	CALLED();
}


void
SoftPipeRenderer::ClearFront(GLcontext *ctx)
{
	CALLED();
}


GLboolean
SoftPipeRenderer::RenderbufferStorage(GLcontext* ctx,
		struct gl_renderbuffer* render, GLenum internalFormat,
		GLuint width, GLuint height)
{
	CALLED();
	return GL_TRUE;
}


void
SoftPipeRenderer::Flush(GLcontext *ctx)
{
	CALLED();
}

