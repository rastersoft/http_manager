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
 * process_list.h
 *
 *  Created on: 5 de oct. de 2015
 *      Author: raster
 */

#ifndef SRC_PROCESS_LIST_H_
#define SRC_PROCESS_LIST_H_

#define PAGE_SIZE 8192

enum Pipe_types {PIPE_MASTER, PIPE_CHILD_STDOUT, PIPE_CHILD_STDERR};

struct Pipe_element {

	int fd;
	int pid;
	char *data;
	int data_size;
	int data_block_size;
	enum Pipe_types type;
	bool to_delete;

	struct Pipe_element *next;
	struct Pipe_element *prev;
};

extern struct Pipe_element pipe_list;
extern struct Pipe_element *pipe_list_last;
extern int pipe_list_size;
extern struct pollfd *fds;

struct Pipe_element * pipe_new(int fd, enum Pipe_types type);
void pipe_delete(struct Pipe_element *element);
void pipe_reset_data(struct Pipe_element *element);
void escape_data(struct Pipe_element *element);
char *escape_string(char *data);
void read_data(struct Pipe_element *object);

#endif /* SRC_PROCESS_LIST_H_ */
