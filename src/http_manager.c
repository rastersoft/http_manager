/*
 * Copyright 2011-12 Raster Software Vigo
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
 * series_rss.c
 *
 *  Created on: 11/12/2011
 *      Author: raster (http://www.rastersoft.com)
 */

#define _BSD_SOURCE

//#define SCRIPTS_PATH "/home/raster/workspace/webtv2/bg_apps/etc/scripts"
#define SCRIPTS_PATH "/etc/scripts"

#include <stdbool.h>

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <regex.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include "http_manager.h"
#include "http.h"


int time_val;
int main_port;
volatile int do_exit;
volatile int do_reload;

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
	free(object->data);
	free(object);
	pipe_list_size--;
	free(fds);
	fds = (struct pollfd *)malloc(pipe_list_size * sizeof(struct pollfd));
	return;
}


void remove_slash_atend(char *cadena,int jump_start) {

	int size;

	size=strlen(cadena);
	if ((size>jump_start)&&(cadena[size-1]=='/')) {
		cadena[size-1]=0;
	}
}


void term_handler_cb(int v) {
	do_exit=1;
}

void run_external_program(struct http_petition *object) {

	char *cadena;
	struct Pipe_element *pobject;
	char cadena2[32];
	int pipefd[2];
	int pid;

	bool piping = object->header[17]=='2';

	if (piping) {
		pipe(pipefd);
	} else {
		pipefd[0] = 0;
	}
	pobject = pipe_new(pipefd[0],PIPE_CHILD);
	if (pobject == NULL) {
		if (piping) {
			close(pipefd[0]);
			close(pipefd[1]);
		}
		http_send_header(object,500,"Not enought memory");
		return;
	}

	pid = fork();
	if (pid == -1) {
		pipe_delete(pobject);
		if (piping) {
			close(pipefd[0]);
			close(pipefd[1]);
		}
		http_send_header(object,500,"Not enought memory");
		return;
	}
	if (pid == 0) {
		int retval;
		if (piping) {
			dup2(pipefd[1],1);
		}
		cadena=malloc(object->data_size+1);
		memcpy(cadena,object->data,object->data_size);
		*(cadena+object->data_size)=0;
		retval = system(cadena);
		free(cadena);
		close(pipefd[1]);
		exit(retval);
	}

	// parent
	pobject->pid = pid;
	http_send_header(object,200,"OK");
	http_send_header_var(object,"Access-Control-Allow-Origin: *");
	sprintf(cadena2,"{ \"pid\" : %d }",pobject->pid);
	http_send_str(object,cadena2);
	http_send_end(object);
	http_free_petition(object);
}


void get_program_result(struct http_petition *object) {

	char cadena2[8192];
	int len;
	int pid;
	bool get_partial;
	int retval, status;
	struct Pipe_element *element;

	char *data;

	if (object->header[15] == '_') {
		pid = atoi(object->header+16);
		get_partial = false;
	} else {
		pid = atoi(object->header+17);
		get_partial = true;
	}

	printf("Pido pid: %d\n",pid);
	element = NULL;
	for(struct Pipe_element *e = pipe_list.next; e!= NULL; e=e->next) {
		if (e->pid == pid) {
			element = e;
			break;
		}
	}
	if (element != NULL) {
		http_send_header(object,200,"OK");
		http_send_header_var(object,"Content-Type: text/plain; charset=UTF-8");
		/* Esta entrada es necesaria para que funcione XMLHttpRequest */
		http_send_header_var(object,"Access-Control-Allow-Origin: *");
		status = waitpid(pid,&retval,WNOHANG);
		char cadena[2048];
		sprintf(cadena,"{ \"running\" : %s,\n\"retval\" : %d,\n\"data\" : \"",status <= 0 ? "true" : "false",retval);
		http_send_str(object,cadena);
		if ((status > 0) || (get_partial)) {
			if (element->data != NULL) {
				http_send_data(object,element->data,element->data_size);
			}
		}
		http_send_str(object,"\"}");
		if (status > 0) {
			element->to_delete = true;
		}
	} else {
		http_send_header(object,503,"PID not valid");
	}

	http_send_end(object);
	http_free_petition(object);
}

void get_files(struct http_petition *object) {

	DIR *dlist;
	struct dirent *dent;
	char *path,first,*path2,tmpstr[50];
	struct stat datos;
	int path_size,file_size;

	http_send_header(object,200,"OK");
	http_send_header_var(object,"Content-Type: text/plain; charset=UTF-8");
	http_send_header_var(object,"Access-Control-Allow-Origin: *");

	path=malloc(object->data_size+2);
	if (path==NULL) {
		dlist=NULL;
	} else {
		memcpy(path,object->data,object->data_size);
		*(path+object->data_size)=0;
		path_size=object->data_size;
		dlist=opendir(path);
		if ((path_size==0) || (path[path_size-1]!='/')) {
			path[path_size]='/';
			path[path_size+1]=0;
			path_size++;
		}
	}
	if (dlist==NULL) {
		http_send_str(object,"{ status: -1, files : []}");
	} else {
		http_send_str(object,"{ status: 0 , files: [");
		first=0;
		do {
			dent=readdir(dlist);
			if (dent!=NULL) {
				if ((dent->d_type!=DT_REG)&&(dent->d_type!=DT_DIR)) {
					continue;
				}
				if (dent->d_name[0]=='.') {
					continue;
				}
				file_size=strlen(dent->d_name);
				path2=malloc(path_size+file_size+1);
				sprintf(path2,"%s%s",path,dent->d_name);
				lstat(path2,&datos);
				if (first) {
					http_send_str(object,", ");
				}
				first=1;
				http_send_str(object,"{ filename:\"");
				http_send_str(object,dent->d_name);
				sprintf(tmpstr,"\", date: %lld , isdir: %s}",(long long int)datos.st_mtime,(dent->d_type==DT_DIR) ? "true" : "false");
				http_send_str(object,tmpstr);
				free(path2);
			}
		} while(dent);
		http_send_str(object,"]}");
		closedir(dlist);
	}
	free(path);
	http_send_end(object);
	http_free_petition(object);

}

void get_services(struct http_petition *object) {

	char first;

	char cadena2[8192];
	char cadena3[8192];
	DIR *folder;
	struct dirent *entrada;
	struct stat estado;

	http_send_header(object,200,"OK");
	http_send_header_var(object,"Content-Type: text/plain; charset=UTF-8");
	/* Esta entrada es necesaria para que funcione XMLHttpRequest */
	http_send_header_var(object,"Access-Control-Allow-Origin: *");

	folder=opendir(SCRIPTS_PATH);

	if (folder==NULL) {
		http_send_str(object,"{ status: -1, services : []}");
	} else {
		http_send_str(object,"{ status: 0, services : [");
		first=0;
		while((entrada=readdir(folder))!=NULL) {
			if (entrada->d_name[0]=='.') {
				continue;
			}
			if (0==strcmp(entrada->d_name,"tolaunch")) {
				continue;
			}
			if (first) {
				http_send_str(object,", ");
			}
			first=1;
			sprintf(cadena2,"%s/tolaunch/%s",SCRIPTS_PATH,entrada->d_name);
			sprintf(cadena3,"{ service: \"%s\", status: %d }",entrada->d_name,(stat(cadena2,&estado)==0) ? 1 : 0);
			http_send_str(object,cadena3);
		}
		http_send_str(object,"]}");
		closedir(folder);
	}
	http_send_end(object);
	http_free_petition(object);
}


void accept_connection(int sockfd) {

	struct http_petition *object;
	char first;

	printf("Nueva conexion\n");
	object=http_accept_connection(sockfd);
	if (object==NULL) {
		printf("Error while accepting a connection\n");
		return;
	}
	if (object->error==0) {
		if (0==strncmp("POST /run_program",object->header,17)) {
			run_external_program(object);
		} else if (0==strncmp("GET /get_result",object->header,15)) {
			get_program_result(object);
		} else if (0==strncmp("POST /get_files",object->header,15)) {
			get_files(object);
		} else if (0==strncmp("GET /get_services",object->header,17)) {
			get_services(object);
		} else {
			http_send_end(object);
			http_free_petition(object);
		}
	} else {
		printf("Error al aceptar conexion %d\n",object->error);
		http_send_end(object);
		http_free_petition(object);
	}
}

void read_data(struct Pipe_element *object) {

	int size;
	char buffer[8192];

	size = read(object->fd,buffer,8192);
	printf("Leido %d datos\n",size);
	if (size <= 0) {
		return;
	}
	if (object->data == NULL) {
		object->data = (char *)malloc(8192);
		object->data_size = 0;
		object->data_block_size = 8192;
	} else if ((object->data_block_size - object->data_size) < size) {
		object->data_block_size += 8192;
		object->data = realloc(object->data,object->data_block_size);
	}
	memcpy(object->data+object->data_size,buffer,size);
	object->data_size += size;
}

void do_loop() {


	int c,l;
	struct Pipe_element *e,*f;
	struct sockaddr_in serv_addr;

	pipe_list.fd = socket(AF_INET, SOCK_STREAM, 0);
	if (pipe_list.fd < 0) {
		printf("Can't create socket\n");
		pipe_list.fd=0;
	} else {
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(main_port);
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		if (bind(pipe_list.fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
			perror("Can't bind socket\n");
			close(pipe_list.fd);
			pipe_list.fd=0;
		} else {
			if ( 0!= listen(pipe_list.fd,5)) {
				printf("Can't listen on socket\n");
				close(pipe_list.fd);
				pipe_list.fd=0;
			}
		}
	}

	while(!do_exit) {
		for (e = &pipe_list, c = 0; e != NULL; e = e->next, c++) {
			fds[c].fd=e->fd;
			fds[c].events=POLLIN;
			fds[c].revents=0;
		}
		if (0 != poll(fds,pipe_list_size,-1)) {
			l = pipe_list_size;
			for (c=0;c<l;c++) {
				if (fds[c].revents & POLLIN) {
					if (fds[c].fd == pipe_list.fd) {
						accept_connection(fds[c].fd);
					} else {
						for (e = pipe_list.next; e != NULL; e = e->next) {
							if (e->fd == fds[c].fd) {
								read_data(e);
							}
						}
					}
				}
			}
			for (e = pipe_list.next; e != NULL;e = f) {
				f = e->next;
				if(e->to_delete) {
					pipe_delete(e);
				}
			}
		}
	}
	close(pipe_list.fd);
}

int main(int argc, char **argv) {

	int loop,t;
	struct sigaction term_action;
	struct sigaction quit_action;
	struct sigaction int_action;
	struct sigaction hup_action;

	time_val=2; // by default, refresh torrents once each two hours
	main_port = 9088;
	pipe_list.fd = 0; // default port
	pipe_list.next = NULL;
	pipe_list.prev = NULL;
	pipe_list.type = PIPE_MASTER;
	pipe_list_last = &pipe_list;
	pipe_list_size = 1;
	fds = (struct pollfd *)malloc(pipe_list_size * sizeof(struct pollfd));

	for(loop=1;loop<argc;loop++) {

		if (!strncmp(argv[loop],"-h",2)) { // help
			printf("\n  SERIES_RSS 1.3\n\nUsage: series_rss [-h] [-Afile] [-Pport] [-Itime_interval] [-Cconfig_file] [-Ttorrents_folder] [-Ddownloaded_list_file]\n\n");
			printf("-h: shows this help.\n");
			printf("-P port to get/set configuration (default 9089).\n");
			exit(0);
		}

		if (!strncmp(argv[loop],"-P",2)) { // port
			t=atoi(2+argv[loop]);
			if ((t>=1)&&(t<=65535)) {
				main_port=t;
			}
			continue;
		}
	}

	do_exit=0;

	term_action.sa_handler=term_handler_cb;
	term_action.sa_flags=0;
	sigemptyset(&term_action.sa_mask);
	sigaction(SIGTERM,&term_action,NULL);

	quit_action.sa_handler=term_handler_cb;
	quit_action.sa_flags=0;
	sigemptyset(&quit_action.sa_mask);
	sigaction(SIGQUIT,&quit_action,NULL);

	int_action.sa_handler=term_handler_cb;
	int_action.sa_flags=0;
	sigemptyset(&int_action.sa_mask);
	sigaction(SIGINT,&int_action,NULL);

	do_loop();

	return 0;

}

