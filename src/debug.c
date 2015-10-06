/*
 * Copyright 2011-15 Raster Software Vigo
 *
 * This file is part of http_manager, derived from series_rss
 *
 * series_rss is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * series_rss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * debug.c
 *
 *  Created on: 5 de oct. de 2015
 *      Author: raster
 */

#include <stdio.h>

#include "debug.h"

static enum DEBUG_MODE level = DEBUG_WARNING;

void debug(enum DEBUG_MODE mode, char *msg) {

	if (mode < level) {
		return;
	}

	printf(msg);
}

void debug_str(enum DEBUG_MODE mode, char *msg,char *string) {
	if (mode < level) {
		return;
	}
	printf(msg,string);
}

void debug_int(enum DEBUG_MODE mode, char *msg,int value) {
	if (mode < level) {
		return;
	}
	printf(msg,value);
}

void set_debug_level(enum DEBUG_MODE mode) {
	level = mode;
}
