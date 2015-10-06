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
