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
 * http.c
 *
 *  Created on: 19/12/2011
 *      Author: raster (http://www.rastersoft.com)
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#include "http.h"

bool http_command_path_are(struct http_petition *object,char *command, char *path) {

	if (0 != strcmp(command,object->header_command)) {
		return false;
	}
	if (0 != strcmp(path,object->header_path)) {
		return false;
	}
	return true;
}

int http_receive_petition(int sockfd, struct http_petition *object) {

	int rss_block,tmp_size;
	int content_length=-1;
	int header_size=-1;
	char *head_line;

	if(object->header) {
		free(object->header);
	}
	object->header=NULL;
	object->total_size=0;

	rss_block=0;
	object->error=0;
	while((content_length==-1)||(header_size==-1)||((object->total_size-header_size)<content_length)) {
		if (object->total_size==rss_block) {
			rss_block+=RSS_BUFF_SIZE;
			head_line=realloc(object->header,rss_block+1);
			object->header=head_line;
		}
		tmp_size=read(sockfd,object->header+object->total_size,rss_block-object->total_size);
		if (tmp_size<=0) {
			object->header[object->total_size]=0;
			return -1;
		}
		object->total_size+=tmp_size;
		object->header[object->total_size]=0;
		if ((object->check_for_size)&&(content_length==-1)) {
			http_get_data(object,0);
			if (object->data_size!=-1) {
				head_line=strstr(object->header,"\r\nContent-Length: ");
				if (head_line) {
					sscanf(head_line+18,"%d",&content_length);
				} else {
					content_length=0;
				}
				header_size=object->total_size-object->data_size;
			}
		}
	}
	http_fill_header(object);
	return 0;
}

void http_free_header_params(struct http_petition *object) {

	if (object == NULL) {
		return;
	}

	struct http_header_param *c,*n;
	for(c=object->header_params;c != NULL; c = n) {
		n = c->next;
		free(c->key);
		free(c->value);
		free(c);
	}
	object->header_params = NULL;
}

char *add_header_param(struct http_petition *object,char *param_list) {

	char *pointer,*pointer2;
	struct http_header_param *param;

	pointer = strchr(param_list,'&');
	if (pointer != NULL) {
		*pointer = 0;
	}
	param = (struct http_header_param *)malloc(sizeof(struct http_header_param));
	if (param == NULL) {
		return NULL;
	}
	bzero(param,sizeof(struct http_header_param));

	pointer2 = strchr(param_list,'=');
	if (pointer2 == NULL) { // no value
		param->key = strdup(param_list);
	} else {
		*pointer2 = 0;
		param->key = strdup(param_list);
		param->value = strdup(pointer2 + 1);
		*pointer2 = '=';
	}
	param->next = object->header_params;
	object->header_params = param;
	if (pointer != NULL) {
		*pointer = '&';
	}
	return pointer;
}

void http_fill_header(struct http_petition *object) {

	char *pointer1, *pointer2, *pointer3;

	pointer1 = strchr(object->header,' ');
	if (pointer1 == NULL) {
		return;
	}

	pointer3 = strchr(pointer1 + 1,' ');
	if (pointer3 != NULL) {
		*pointer3 = 0;
	}
	free(object->header_command);
	*pointer1 = 0;
	object->header_command = strdup(object->header);
	*pointer1 = ' ';

	pointer2 = strchr(object->header,'?');
	if (pointer2 == NULL) {
		object->header_path = strdup(pointer1+1);
	} else {
		*pointer2 = 0;
		object->header_path = strdup(pointer1+1);
		*pointer2 = '?';
		do {
			pointer2 = add_header_param(object,pointer2 + 1);
		} while(pointer2 != NULL);
	}
	if (pointer3 != NULL) {
		*pointer3 = ' ';
	}
}

void http_get_data(struct http_petition *object, char modify) {

	/* Jump over the HTTP header and return a pointer to the data itself */

	char *p,*q;
	int size;

	if ((object==NULL)||(object->header==NULL)) {
		return;
	}

	object->data_size=-1;
	size=object->total_size;
	for(p=object->header;*p;p++) {
		size--;
		if (*p=='\r') {
			q=p+1;
			if ((*q==0)||(*q!='\n')) {
				continue;
			}
			q++;
			if ((*q==0)||(*q!='\r')) {
				continue;
			}
			if (modify) {
				*(p+2)=0; // end of string at the end of headers (but preserve the last carriage return)
			}
			object->data=p+4;
			object->data_size=size-3;

			return;
		}
		if (*p=='\n') {
			q=p+1;
			if ((*q==0)||(*q!='\n')) {
				continue;
			}
			if (modify) {
				*(q)=0; // end of string at the end of headers (but preserve the last carriage return)
			}
			object->data=p+2;
			object->data_size=size-1;

			return;
		}
	}
}

int http_get_error(struct http_petition *object) {

	int v;

	if (object==NULL) {
		return -1;
	}
	if (strncmp(object->header,"HTTP/1.",7)) {
		return -2;
	}

	sscanf(object->header+9,"%d",&v);
	return (v);
}

char *http_tobase64(char *input) {

	char *data="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char buffer[3];
	int size,counter,atend;
	char *p1,*p2,*p3;
	char *output;

	size=strlen(input);
	output=malloc(size*2);
	for(p1=input,p3=output;*p1;) {
		atend=0;
		for(p2=buffer,counter=0;counter<3;counter++,p2++) {
			*p2=*p1;
			if (*p1) {
				p1++;
			} else {
				atend++;
			}
		}
		*(p3++)=data[(int)((buffer[0]>>2)&0x3F)];
		*(p3++)=data[(int)((((buffer[0]<<4)&0x30)|((buffer[1]>>4)&0x0F)))];
		if (atend<2) {
			*(p3++)=data[(int)((((buffer[1]<<2)&0x3C)|((buffer[2]>>6)&0x03)))];
		} else {
			*(p3++)='=';
		}
		if (atend==0) {
			*(p3++)=data[(int)(buffer[2]&0x3F)];
		} else {
			*(p3++)='=';
		}
		*p3=0;
	}
	return output;
}

void http_free_petition(struct http_petition *object) {

	if (object==NULL) {
		return;
	}

	free(object->session_id);
	free(object->header);
	free(object->userpass);
	free(object->header_command);
	free(object->header_path);

	http_free_buffer(&object->header_buffer);
	http_free_buffer(&object->data_buffer);
	http_free_header_params(object);

	free(object);
}

void http_alarm_cb(int v) {

	printf("Timeout\n");
}


/********************************************************************
 * This function accepts a HTTP connection at the socket SOCKFD,    *
 * reads the header and, optionally, the data sent by the browser,  *
 * and returns it in the OBJECT structure. Also returns the new     *
 * socket.                                                          *
 ********************************************************************/

struct http_petition * http_accept_connection(int sockfd) {

	int newsockfd;
	socklen_t clen;
	struct sockaddr_in cli_addr;
	struct http_petition *object;
	char *p;

	object=malloc(sizeof(struct http_petition));
	if(object==NULL) {
		return NULL;
	}
	bzero(object,sizeof(struct http_petition));

	clen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clen);
	if (newsockfd < 0) {
		printf("Can't accept connection\n");
		object->error=1;
		return object;
	}

	object->error=0;
	object->check_for_size=1;
	http_receive_petition(newsockfd,object);
	http_get_data(object,1);
	object->sockfd=newsockfd;
	object->http_version='0';
	p=strstr(object->header,"HTTP/1.");
	if (p!=NULL) {
		object->http_version=*(p+7);
	}
	return object;
}

void http_append_data(struct dinstr *object,char *string,int size) {

	if ((object==NULL)||(size==0)) {
		return;
	}

	while ((object->size+size+1)>object->fullsize) {
		object->fullsize+=RSS_BUFF_SIZE;
		object->buffer=realloc(object->buffer,object->fullsize);
	}

	memcpy(object->buffer+object->size,string,size);
	object->size+=size;
	*(object->buffer+object->size)=0;
}

void http_append_string(struct dinstr *object,char *string) {

	http_append_data(object,string,strlen(string));

}

void http_free_buffer(struct dinstr *object) {

	if (object == NULL) {
		return;
	}
	free(object->buffer);
	object->buffer=NULL;
	object->size=0;
	object->fullsize=0;
}

void http_send_end(struct http_petition *object) {

	char cadena[1024];
	if (object->data_buffer.size>0) {
		sprintf(cadena,"Content-Length: %d\r\n\r\n",object->data_buffer.size);
		http_append_string(&object->header_buffer,cadena);
	} else {
		http_append_string(&object->header_buffer,"\r\n");
	}

	//printf("Envio de vuelta: %s\n\n",object->header_buffer.buffer);

	write(object->sockfd,object->header_buffer.buffer,object->header_buffer.size);
	if (object->data_buffer.size>0) {
		write(object->sockfd,object->data_buffer.buffer,object->data_buffer.size);
		//printf("%s",object->data_buffer.buffer);
	}
	//printf("\n");
	http_free_buffer(&object->header_buffer);
	http_free_buffer(&object->data_buffer);
	close(object->sockfd);

}

void http_send_str(struct http_petition *object,char *str) {

	http_append_string(&object->data_buffer,str);

}

void http_send_data(struct http_petition *object, char *data, int size) {

	http_append_data(&object->data_buffer,data,size);

}


void http_send_header(struct http_petition *object, int error,char *str_error) {

	char str[256];

	if (object->http_version=='1') {
		sprintf(str,"HTTP/1.1 %d %s\r\nConnection: close\r\n",error,str_error);
	} else {
		sprintf(str,"HTTP/1.0 %d %s\r\n",error,str_error);
	}
	http_append_string(&object->header_buffer,str);
}

void http_send_header_var(struct http_petition *object, char *var) {

	http_append_string(&object->header_buffer,var);
	http_append_string(&object->header_buffer,"\r\n");

}
