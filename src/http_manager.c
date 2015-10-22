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
 * http_manager.c
 *
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
#include <netinet/in.h>
#include <arpa/inet.h>

#include "http_manager.h"
#include "http.h"
#include "debug.h"
#include "process_list.h"

int time_val;
int main_port;
volatile int do_exit;
volatile int do_reload;


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

void run_external_program(struct http_petition *object,bool piping) {

    char *cadena;
    struct Pipe_element *pobject_out;
    struct Pipe_element *pobject_err;
    char cadena2[32];
    int pipefd_out[2];
    int pipefd_err[2];
    int pid;

    if (piping) {
        pipe(pipefd_out);
        pipe(pipefd_err);
    } else {
        pipefd_out[0] = 0;
        pipefd_err[0] = 0;
    }
    pobject_out = pipe_new(pipefd_out[0],PIPE_CHILD_STDOUT);
    pobject_err = pipe_new(pipefd_err[0],PIPE_CHILD_STDERR);
    if ((pobject_out == NULL) ||(pobject_err == NULL)) {
        pipe_delete(pobject_out);
        pipe_delete(pobject_err);
        if (piping) {
            close(pipefd_out[0]);
            close(pipefd_out[1]);
            close(pipefd_err[0]);
            close(pipefd_err[1]);
        }
        http_send_header(object,500,"Not enought memory");
        debug(DEBUG_ERROR,"Not enough memory for creating a new process in the system\n");
        return;
    }

    pid = fork();
    if (pid == -1) {
        pipe_delete(pobject_out);
        pipe_delete(pobject_err);
        if (piping) {
            close(pipefd_out[0]);
            close(pipefd_out[1]);
            close(pipefd_err[0]);
            close(pipefd_err[1]);
        }
        http_send_header(object,500,"Not enought memory");
        debug(DEBUG_ERROR,"Not enough memory for creating a child\n");
        return;
    }
    if (pid == 0) {
        int retval;
        if (piping) {
            dup2(pipefd_out[1],1);
            dup2(pipefd_err[1],2);
        }
        cadena=malloc(object->data_size+1);
        memcpy(cadena,object->data,object->data_size);
        *(cadena+object->data_size)=0;
        debug_str(DEBUG_INFO,"Launching %s\n",cadena);
        retval = system(cadena);
        free(cadena);
        if (piping) {
            close(pipefd_out[1]);
            close(pipefd_err[1]);
        }
        exit(retval);
    }

    // parent
    pobject_out->pid = pid;
    pobject_err->pid = pid;
    http_send_header(object,200,"OK");
    http_send_header_var(object,"Access-Control-Allow-Origin: *");
    sprintf(cadena2,"{ \"pid\" : %d }",pid);
    http_send_str(object,cadena2);
    http_send_end(object);
    http_free_petition(object);
}

void escape_data(struct Pipe_element *element) {

    if (element->data_size == 0) {
        return;
    }
    int counter;
    int loop;
    char *p;
    char *q;
    char c;
    
    counter = 0;
    
    for(loop=0,p=element->data; loop<element->data_size;loop++,p++) {
        c = *p;
        if ((c == '"') || (c == '\n') || (c == '\r') || (c == '\t')) {
            counter++;
        }
    }
    if (counter != 0) {
        element->data = realloc(element->data,element->data_size+counter);
        p = element->data+element->data_size+counter-1;
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
            default:
                *p = c;
            break;
            }
        }
        element->data_size+=counter;
    }
}

void get_program_result(struct http_petition *object, bool get_partial) {

    int pid;
    int retval, status;
    struct Pipe_element *element_out, *element_err, *e;

    struct http_header_param *param;

    pid = -1;
    for (param = object->header_params; param!= NULL; param = param->next) {
        if (0 == strcmp(param->key,"pid")) {
            if (param->value != NULL) {
                pid = atoi(param->value);
                break;
            }
        }
    }

    debug_int(DEBUG_INFO, "Pido PID %d\n",pid);
    element_out = NULL;
    element_err = NULL;
    for(e = pipe_list.next; e!= NULL; e=e->next) {
        if (e->pid == pid) {
            if (e->type == PIPE_CHILD_STDOUT) {
                element_out = e;
            } else if (e->type == PIPE_CHILD_STDERR) {
                element_err = e;
            }
            if ((element_out != NULL) && (element_err != NULL)) {
                break;
            }
        }
    }
    if (element_out != NULL) {
        http_send_header(object,200,"OK");
        http_send_header_var(object,"Content-Type: text/plain; charset=UTF-8");
        /* Esta entrada es necesaria para que funcione XMLHttpRequest */
        http_send_header_var(object,"Access-Control-Allow-Origin: *");
        status = waitpid(pid,&retval,WNOHANG);
        char cadena[2048];
        sprintf(cadena,"{ \"running\" : %s, \"retval\" : %d, \"stdout\" : \"",status <= 0 ? "true" : "false",retval);
        http_send_str(object,cadena);
        if ((status > 0) || (get_partial)) {
            if (element_out->data != NULL) {
                escape_data(element_out);
                http_send_data(object,element_out->data,element_out->data_size);
            }
        }
        http_send_str(object,"\", \"stderr\" : \"");
        if ((status > 0) || (get_partial)) {
            if (element_err->data != NULL) {
                escape_data(element_err);
                http_send_data(object,element_err->data,element_err->data_size);
            }
        }
        http_send_str(object,"\"}");
        if (status > 0) {
            element_out->to_delete = true;
            element_err->to_delete = true;
        }
        if (get_partial) {
            pipe_reset_data(element_out);
            pipe_reset_data(element_err);
        }
    } else {
        debug(DEBUG_ERROR, "Asking for not valid PID\n");
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

    debug(DEBUG_DEBUG, "New conection\n");
    object=http_accept_connection(sockfd);
    if (object==NULL) {
        debug(DEBUG_ERROR, "Can't open a new conection\n");
        return;
    }
    if (object->error==0) {
        debug_str(DEBUG_DEBUG,"Command: %s\n",object->header_command);
        debug_str(DEBUG_DEBUG,"Path: %s\n",object->header_path);
        if (http_command_path_are(object,"POST", "/run_program")) {
            run_external_program(object,false);
        } else if (http_command_path_are(object,"POST", "/run_program_with_pipes")) {
            run_external_program(object,true);
        } else if (http_command_path_are(object,"GET","/get_result")) {
            get_program_result(object,false);
        } else if (http_command_path_are(object,"GET","/get_partial_result")) {
            get_program_result(object,true);
        } else if (http_command_path_are(object,"POST","/get_files")) {
            get_files(object);
        } else if (http_command_path_are(object,"GET","/get_services")) {
            get_services(object);
        } else {
            http_send_header(object,501,"COMMAND NOT VALID");
            http_send_header_var(object,"Content-Type: text/plain; charset=UTF-8");
            /* Esta entrada es necesaria para que funcione XMLHttpRequest */
            http_send_header_var(object,"Access-Control-Allow-Origin: *");
            http_send_end(object);
            http_free_petition(object);
        }
    } else {
        debug_int(DEBUG_ERROR, "Error when accepting the conection\n",object->error);
        http_send_header(object,500,"INTERNAL ERROR");
        http_send_header_var(object,"Content-Type: text/plain; charset=UTF-8");
        /* Esta entrada es necesaria para que funcione XMLHttpRequest */
        http_send_header_var(object,"Access-Control-Allow-Origin: *");
        http_send_end(object);
        http_free_petition(object);
    }
}

void read_data(struct Pipe_element *object) {

    int size;
    char buffer[8192];

    size = read(object->fd,buffer,8192);
    debug_int(DEBUG_DEBUG, "Read %d bytes\n",size);
    printf("Leido %d datos\n",size);
    if (size <= 0) {
        debug(DEBUG_ERROR, "Error while reading data\n");
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
        debug(DEBUG_CRITICAL, "Error while creating the main socket\n");
        exit(-1);
    } else {
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(main_port);
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        if (bind(pipe_list.fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("Can't bind socket\n");
            close(pipe_list.fd);
            pipe_list.fd=0;
            exit(-1);
        } else {
            if ( 0!= listen(pipe_list.fd,5)) {
                printf("Can't listen on socket\n");
                close(pipe_list.fd);
                pipe_list.fd=0;
                exit(-1);
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

    set_debug_level(DEBUG_INFO);

    main_port = 9089;
    pipe_list.fd = 0; // default port
    pipe_list.next = NULL;
    pipe_list.prev = NULL;
    pipe_list.type = PIPE_MASTER;
    pipe_list_last = &pipe_list;
    pipe_list_size = 1;
    fds = (struct pollfd *)malloc(pipe_list_size * sizeof(struct pollfd));

    for(loop=1;loop<argc;loop++) {

        if (!strncmp(argv[loop],"-h",2)) { // help
            printf("\n  HTTP_MANAGER 1.1\n\nUsage: http_manager [-h] [-Pport]\n\n");
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
