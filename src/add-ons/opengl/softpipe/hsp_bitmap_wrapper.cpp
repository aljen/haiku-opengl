/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on gallium gdi winsys
 *
 * Authors:
 * 	Artur Wyszynski <harakash@gmail.com>
 */

#include <stdio.h>
#include <interface/Bitmap.h>
#include "hsp_bitmap_wrapper.h"

extern "C" {

Bitmap*
create_bitmap(int32 width, int32 height, color_space colorSpace)
{
	BBitmap *bb = new BBitmap(BRect(0, 0, width, height), colorSpace);
	if (bb)
		return (Bitmap*)bb;
	return NULL;
}

void
get_bitmap_size(const Bitmap *bitmap, int32 *width, int32 *height)
{
	BBitmap *bb = (BBitmap*)bitmap;
	if (bb && width && height) {
		uint32 w = bb->Bounds().Width();
		uint32 h = bb->Bounds().Height();
		*width = w;
		*height = h;
	}
}

color_space
get_bitmap_color_space(const Bitmap *bitmap)
{
	BBitmap *bb = (BBitmap*)bitmap;
	if (bb)
		return bb->ColorSpace();
	return B_NO_COLOR_SPACE;
}

void
copy_bitmap_bits(const Bitmap *bitmap, void *data, int32 length)
{
	BBitmap *bb = (BBitmap*)bitmap;
	if (bb) {
		color_space cs = bb->ColorSpace();
		bb->ImportBits(data, bb->BitsLength(), bb->BytesPerRow(), 0, cs);
	}
}

void
delete_bitmap(Bitmap *bitmap)
{
	BBitmap *bb = (BBitmap*)bitmap;
	delete bb;
}

int32
get_bitmap_bytes_per_row(const Bitmap *bitmap)
{
	BBitmap *bb = (BBitmap*)bitmap;
	if (bb)
		return bb->BytesPerRow();
	return 0;
}

int32
get_bitmap_bits_length(const Bitmap *bitmap)
{
	BBitmap *bb = (BBitmap*)bitmap;
	if (bb)
		return bb->BitsLength();
	return 0;
}

}
