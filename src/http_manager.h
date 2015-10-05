/*
 * http_manager.h
 *
 *  Created on: 4 de oct. de 2015
 *      Author: raster
 */

#ifndef SRC_HTTP_MANAGER_H_
#define SRC_HTTP_MANAGER_H_

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

struct Pipe_element * pipe_new(int fd, enum Pipe_types type);
void pipe_delete(struct Pipe_element *element);
void pipe_reset_data(struct Pipe_element *element);

#endif /* SRC_HTTP_MANAGER_H_ */
