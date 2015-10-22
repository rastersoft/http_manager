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

