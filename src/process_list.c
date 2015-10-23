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
#include <string.h>

#include "process_list.h"
#include "debug.h"

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

void escape_data(struct Pipe_element *element) {

    if (element->data_size == 0) {
        return;
    }
    int counter;
    int loop;
    int final_size;
    char *p;
    char *q;
    char c;
    
    counter = 0;
    
    for(loop=0,p=element->data; loop<element->data_size;loop++,p++) {
        c = *p;
        if ((c == '"') || (c == '\n') || (c == '\r') || (c == '\t') || (c == '\\')) {
            counter++;
        }
    }
    if (counter != 0) {
        final_size = element->data_size + counter;
        if (element->data_block_size < final_size) {
            do {
                element->data_block_size += PAGE_SIZE;
            } while (element->data_block_size < final_size);
            element->data = realloc(element->data,element->data_block_size);
        }
        p = element->data+final_size-1;
        q = element->data+element->data_size-1;
        for(loop=0;loop<element->data_size;loop++,p--,q--) {
            c = *q;
            switch(c) {
            case '"':
                *p = '"';
                p--;
                *p='\\';
            break;
            case '\n':
                *p = 'n';
                p--;
                *p='\\';
            break;
            case '\r':
                *p = 'r';
                p--;
                *p='\\';
            break;
            case '\t':
                *p = 't';
                p--;
                *p='\\';
            break;
            case '\\':
                *p = '\\';
                p--;
                *p='\\';
            break;
            default:
                *p = c;
            break;
            }
        }
        element->data_size = final_size;
    }
}

char *escape_string(char *data) {

    struct Pipe_element element;

    element.data = strdup(data);
    element.data_size = 1 + strlen(data);
    element.data_block_size = element.data_size;
    escape_data(&element);
    return (element.data);
}

void read_data(struct Pipe_element *object) {

    int size;
    char buffer[PAGE_SIZE];

    size = read(object->fd,buffer,PAGE_SIZE);
    debug_int(DEBUG_DEBUG, "Read %d bytes\n",size);
    if (size <= 0) {
        debug(DEBUG_ERROR, "Error while reading data\n");
        return;
    }
    if (object->data == NULL) {
        object->data = (char *)malloc(PAGE_SIZE);
        object->data_size = 0;
        object->data_block_size = PAGE_SIZE;
    } else if ((object->data_block_size - object->data_size) < size) {
        object->data_block_size += PAGE_SIZE;
        object->data = realloc(object->data,object->data_block_size);
    }
    memcpy(object->data+object->data_size,buffer,size);
    object->data_size += size;
}
