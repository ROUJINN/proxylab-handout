#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void sigchld_handler(int sig) { // reap all children
    int bkp_errno = errno;
    while(waitpid(-1, NULL, WNOHANG)>0);
    errno=bkp_errno;
}

/*如果成功，返回0，否则为-1*/
int parse_url(char *url, char *host, char *port, char *uri) {
    /*http://localhost:8000/home.html*/
    char *p1 = strstr(url,"//");
    if (p1 == NULL) {
        return -1;
    }
    /* 默认前面是http */
    char *p2 = index(p1+2,':');
    /* 没有端口号，则设为80 */
    if (p2 == NULL) {
        port = "80";
        char *p3 = index(p1+2,'/');
        *p3 = '\0';
        strcpy(host,p1+2);
        strcat(uri,"/");
        strcat(uri,p3+1);
        return 0;        
    }
    else {
        int port_number;
        sscanf(p2+1,"%d%s",&port_number,uri);
        sprintf(port, "%d", port_number);
        *p2 = '\0';
        strcpy(host,p1+2);
        return 0;
    }
}

void build_header(rio_t *client_rio, char *header, char *host, char *port) {
    char name[MAXLINE],data[MAXLINE],client_buf[MAXLINE];
    int has_host = 0;
    while(Rio_readlineb(client_rio, client_buf, MAXLINE)) {
        if (strcasecmp(client_buf, "\r\n")) {
            break;
        }
        scanf(client_buf,"%s %s",name,data);
        if (!strcasecmp(name,"Host:")) {
            strcat(header, client_buf);
            has_host = 1;
        }
        if (strcasecmp(name,"User-Agent:") && strcasecmp(name,"Connection:") && strcasecmp(name,"Proxy-Connection:")) {
            strcat(header, client_buf);
        }
    }
    if (!has_host) {
        sprintf(header, "%sHost: %s:%s\r\n", header, host, port);
    }
    strcat(header, user_agent_hdr);
    strcat(header, "Connection: close\r\n");
    strcat(header, "Proxy-Connection: close\r\n");
    strcat(header,"\r\n");
    return;
}

void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-overflow" 
        /* Build the HTTP response body */
        sprintf(body, "<html><title>Tiny Error</title>");
        sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
        sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
        sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
        sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
    #pragma GCC diagnostic pop

        /* Print the HTTP response */
        sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
        rio_writen(fd, buf, strlen(buf));
        sprintf(buf, "Content-type: text/html\r\n");
        rio_writen(fd, buf, strlen(buf));
        sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
        rio_writen(fd, buf, strlen(buf));
        rio_writen(fd, body, strlen(body));
}
/* $end clienterror */

/*和client建立连接后，作为一个client去和服务器连接*/
void doit(int fd) {

    int server_fd;
    rio_t client_rio, server_rio;
    char client_buf[MAXLINE], server_buf[MAXLINE];
    char method[MAXLINE], url[MAXLINE], version[MAXLINE]; 
    char host[MAXLINE], port[MAXLINE], uri[MAXLINE];
    char header[MAXLINE]; 
    //char object[MAX_OBJECT_SIZE];  

    Rio_readinitb(&client_rio, fd);
    Rio_readlineb(&client_rio, client_buf, MAXLINE);

    sscanf(client_buf, "%s %s %s", method, url, version);

    if (strcasecmp(method, "GET")) { 
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }

    parse_url(url,host,port,uri);

    sprintf(header, "%s %s HTTP/1.0\r\n",method,uri);
    build_header(&client_rio, header, host, port);
    printf("!!!%s",header);

    server_fd = Open_clientfd(host, port);
    
    Rio_readinitb(&server_rio, server_fd);
    Rio_writen(server_fd, header, strlen(header));

     /*貌似也可以直接用Rio_readn，这里有个bug十分隐晦，不能写strlen(server_buf)，因为为0*/
    ssize_t n;
    while (n = Rio_readnb(&server_rio, server_buf, MAXLINE)) {
        Rio_writen(fd, server_buf, n);
    }

    Close(server_fd);
}


/*作为一个server，监听client来的连接请求，并且建立连接*/
int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, sigchld_handler);

    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(1);
    }

    listenfd = open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen); 
        getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                        port, MAXLINE, 0);
        doit(connfd);                     
        close(connfd);                                         
    }
}

