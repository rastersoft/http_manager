/*
 * process_list.h
 *
 *  Created on: 5 de oct. de 2015
 *      Author: raster
 */

#ifndef SRC_PROCESS_LIST_H_
#define SRC_PROCESS_LIST_H_

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

#endif /* SRC_PROCESS_LIST_H_ */
