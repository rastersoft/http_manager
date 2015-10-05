/*
 * process_list.c
 *
 *  Created on: 5 de oct. de 2015
 *      Author: raster
 */

#include <stdlib.h>
#include <stdbool.h>
#include <poll.h>
#include <unistd.h>

#include "process_list.h"

struct Pipe_element pipe_list;
struct Pipe_element *pipe_list_last;
int pipe_list_size;
struct pollfd *fds = NULL;

struct Pipe_element * pipe_new(int fd, enum Pipe_types type) {

	struct Pipe_element *object = (struct Pipe_element *)malloc(sizeof(struct Pipe_element));
	if (object == NULL) {
		return NULL;
	}
	object->to_delete = false;
	object->type = type;
	object->fd = fd;
	object->next = NULL;
	object->prev = pipe_list_last;
	object->data = NULL;
	object->data_size = 0;
	object->data_block_size = 0;
	pipe_list_last->next = object;
	pipe_list_last = object;
	pipe_list_size++;
	free(fds);
	fds = (struct pollfd *)malloc(pipe_list_size * sizeof(struct pollfd));
	return object;
}

void pipe_reset_data(struct Pipe_element *object) {
	if (object == NULL) {
		return;
	}
	free(object->data);
	object->data = NULL;
	object->data_block_size = 0;
	object->data_size = 0;
}

void pipe_delete(struct Pipe_element *object) {

	if (object == NULL) {
		return;
	}

	if (object == &pipe_list) {
		return;
	}
	object->prev->next = object->next;
	if (object->next != NULL) {
		object->next->prev = object->prev;
	}
	if (object->fd != 0) {
		close(object->fd);
	}
	if (pipe_list_last == object) {
		pipe_list_last = object->prev;
	}
	pipe_reset_data(object);
	free(object);
	pipe_list_size--;
	free(fds);
	fds = (struct pollfd *)malloc(pipe_list_size * sizeof(struct pollfd));
	return;
}

