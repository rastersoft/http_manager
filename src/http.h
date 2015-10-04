/*
 * Copyright 2011 Raster Software Vigo
 *
 * This file is part of series_rss
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
 * http.h
 *
 *  Created on: 19/12/2011
 *      Author: raster (http://www.rastersoft.com)
 */

#ifndef HTTP_H_
#define HTTP_H_

#define RSS_TIMEOUT 10
#define RSS_BUFF_SIZE 4096

struct dinstr {

	char *buffer;
	int size;
	int fullsize;
};

struct http_extra_data {
	char *user;
	char *pass;
	char *header;
	char *data;
	int data_size;
};

struct http_petition {

	char *userpass;
	char *data;
	char *header;
	char *session_id;
	int error;
	int http_error;
	int data_size;
	int total_size;
	char check_for_size;
	int sockfd;
	char http_version;
	struct dinstr header_buffer;
	struct dinstr data_buffer;
	struct http_extra_data *extra;
};

void http_connect(char *url, struct http_petition *);
struct http_petition *http_accept_connection(int sockfd);
void http_send_header(struct http_petition *object, int error,char *str_error);
void http_send_header_var(struct http_petition *object, char *var);
void http_send_str(struct http_petition *object,char *str);
void http_send_data(struct http_petition *object, char *data, int size);
void http_send_end(struct http_petition *object);
void http_get_data(struct http_petition *, char modify);
int http_receive_petition(int sockfd, struct http_petition *object);

struct http_petition *http_do_petition(char *url, struct http_extra_data *extra);
void http_free_petition(struct http_petition *);

char *http_tobase64(char *input);

#endif /* HTTP_H_ */
