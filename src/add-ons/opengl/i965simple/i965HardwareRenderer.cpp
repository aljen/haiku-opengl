/*
 * Copyright 2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Screen.h>
#include <GLRenderer.h>

extern "C" _EXPORT BGLRenderer * 
instanciate_gl_renderer(BGLView *view, ulong options, BGLDispatcher *dispatcher)
{
	if (!view)
		return NULL;

	BWindow *window = view->Window();
	if (!window)
		return NULL;
	
	BScreen screen(window);
	if (!screen.IsValid())
		return NULL;
	
	accelerant_device_info adi;
	if (screen.GetDeviceInfo(&adi) !=B_OK)
		return NULL;
	
	fprintf(stderr, "Accelerant device info:\n"
			"  version: %ud\n"
			"  name:    %s\n"
			"  chipset: %s\n"
			"  serial#: %s\n", (unsigned int) adi.version, adi.name, adi.chipset, adi.serial_no);
			
	// Check the view is attached to a screen driven by a NVidia chip:
	if (strncmp(adi.name, "", 6) == 0) {
		//return new i965Renderer(view, options, dispatcher);
		return NULL;
	}
	
	return NULL;
}

