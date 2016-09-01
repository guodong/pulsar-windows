#include "stdafx.h"
#include "cip_window.h"

cip_window_t *cip_window_create(uint32_t wid, int16_t x, int16_t y, uint16_t width, uint16_t height, uint32_t flags)
{
	cip_window_t *window = (cip_window_t*)malloc(sizeof(cip_window_t));
	window->wid = wid;
	window->x = x;
	window->y = y;
	window->width = width;
	window->height = height;
	window->bare = flags & CIP_WINDOW_BARE;
	window->visible = flags & CIP_WINDOW_VISIBLE;
	return window;
}