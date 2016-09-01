#pragma once
#include <stdint.h>

#define CIP_WINDOW_BARE 0x1
#define CIP_WINDOW_VISIBLE 0x2

typedef struct {
	int wid;
	uint8_t bare;
	uint8_t visible;
	int16_t x;
	int16_t y;
	uint16_t width;
	uint16_t height;
} cip_window_t;

cip_window_t *cip_window_create(uint32_t wid, int16_t x, int16_t y, uint16_t width, uint16_t height, uint32_t flags);