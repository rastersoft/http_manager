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
