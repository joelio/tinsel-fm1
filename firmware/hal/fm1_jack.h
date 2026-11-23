/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Joel Merrick */
/* The headphone jack's switch (Tinsel): PB5 reads 0 with headphones in, 1 with nothing plugged; the speaker mutes
 * itself in hardware. Read only: the pin is left as the boot leaves it. The fact is felucca-bsp's (boards/fm1/
 * fm1_jack.h, measured on an FM-1 on 29 October 2026); it bounces for about 150 ms a plug, so debounce it.
 *
 *   fm1_hp_in()   1: headphones in (raw) */
#pragma once
#include <stdint.h>
#include "fm1_gpio.h"

static inline uint32_t fm1_hp_in(void) { return !(FM1_PR(FM1_PB, FM1_IN) >> 5 & 1u); }
