/*
 * debug.h
 *
 *  Created on: 5 de oct. de 2015
 *      Author: raster
 */

#ifndef SRC_DEBUG_H_
#define SRC_DEBUG_H_

enum DEBUG_MODE {DEBUG_INFO, DEBUG_WARNING, DEBUG_ERROR, DEBUG_CRITICAL};

void debug(enum DEBUG_MODE, char *msg);
void debug_str(enum DEBUG_MODE, char *msg,char *string);
void debug_int(enum DEBUG_MODE, char *msg,int value);
void set_debug_level(enum DEBUG_MODE);

#endif /* SRC_DEBUG_H_ */
