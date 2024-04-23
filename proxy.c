#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
void read_requesthdrs(rio_t *rp, char *fd, char *request_buf, char *hostname, char *port);
int read_responsehdrs(rio_t *rp, char *fd);
int parse_uri(char *uri, char *filename, char *hostname, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

void doit(int ctopfd)
{
  char *ptosfd;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], hostname[MAXLINE], port[MAXLINE], request_buf[MAXLINE];
  int content_length;
  rio_t client_rio, server_rio; // 클라이언트와 통신하는 리오, 엔드 서버와 통신하는 리오

  /*Read request line and headers*/
  Rio_readinitb(&client_rio, ctopfd);
  Rio_readlineb(&client_rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  if(!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)) {
    clienterror(ctopfd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  if(!strcasecmp(uri, "/favicon.ico")) {
    printf("ignore favicon\n");
    return;
  }
  parse_uri(uri, filename, hostname, port);
  printf("%s\n%s %s %s %s %s\n", uri, method, hostname, version, port, filename);


  // 엔드 서버랑 통신
  sprintf(request_buf, "%s %s %s\r\n", method, filename, "HTTP/1.0");
  ptosfd = Open_clientfd(hostname, port);
  Rio_writen(ptosfd, request_buf, strlen(request_buf));
  read_requesthdrs(&client_rio, ptosfd, request_buf, hostname, port);  // 클라이언트로부터 받은 헤더 엔드서버로 전송

  // 엔드서버로부터 받은 리스폰스 처리

  // 리스폰스 헤더 받고 content-length 저장
  Rio_readinitb(&server_rio, ptosfd);
  content_length = read_responsehdrs(&server_rio, ctopfd);
  printf("%d\n", content_length);
  
  // 리스폰스 바디 받아야함
  char saver[MAXLINE];
  ssize_t n;
  ssize_t size = 0;
  while((n = Rio_readnb(&server_rio, saver, MAXLINE)) > 0)
  {
    Rio_writen(ctopfd, saver, n);
    size += n;
  }
  printf("sending msg size: %ld\n", size);
  // char *response_ptr = malloc(content_length);
  // Rio_readnb(&server_rio, response_ptr, content_length);
  // Rio_writen(ctopfd, response_ptr, content_length); // Client에 Response Body 전송
  // free(response_ptr); // 캐싱하지 않은 경우만 메모리 반환

  Close(ptosfd);


  // read_requesthdrs(&server_rio, ctopfd);
  

  /* Parse URI from GET request */

}




// 헤더를 받아서 보내주는 함수(rp를 읽어서 fd에 보내줌)
void read_requesthdrs(rio_t *rp, char *fd, char *request_buf, char *hostname, char *port)
{
  int is_host_exist;
  int is_connection_exist;
  int is_proxy_connection_exist;
  int is_user_agent_exist;

  Rio_readlineb(rp, request_buf, MAXLINE);
  while (strcmp(request_buf, "\r\n"))
  {
    if (strstr(request_buf, "Proxy-Connection") != NULL)
    {
      sprintf(request_buf, "Proxy-Connection: close\r\n");
      is_proxy_connection_exist = 1;
    }
    else if (strstr(request_buf, "Connection") != NULL)
    {
      sprintf(request_buf, "Connection: close\r\n");
      is_connection_exist = 1;
    }
    else if (strstr(request_buf, "User-Agent") != NULL)
    {
      sprintf(request_buf, user_agent_hdr);
      is_user_agent_exist = 1;
    }
    else if (strstr(request_buf, "Host") != NULL)
    {
      is_host_exist = 1;
    }

    Rio_writen(fd, request_buf, strlen(request_buf)); // Server에 전송
    Rio_readlineb(rp, request_buf, MAXLINE);       // 다음 줄 읽기
  }

  // 필수 헤더 미포함 시 추가로 전송
  if (!is_proxy_connection_exist)
  {
    sprintf(request_buf, "Proxy-Connection: close\r\n");
    Rio_writen(fd, request_buf, strlen(request_buf));
  }
  if (!is_connection_exist)
  {
    sprintf(request_buf, "Connection: close\r\n");
    Rio_writen(fd, request_buf, strlen(request_buf));
  }
  if (!is_host_exist)
  {
    sprintf(request_buf, "Host: %s:%s\r\n", hostname, port);
    Rio_writen(fd, request_buf, strlen(request_buf));
  }
  if (!is_user_agent_exist)
  {
    sprintf(request_buf, user_agent_hdr);
    Rio_writen(fd, request_buf, strlen(request_buf));
  }

  sprintf(request_buf, "\r\n"); // 종료문
  Rio_writen(fd, request_buf, strlen(request_buf));
  return;
}

// 리스폰스 헤더 읽고 content-length 리턴
int read_responsehdrs(rio_t *rp, char *fd)
{
  char buf[MAXLINE];
  char *ptr = NULL;
  int content_length;
  

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) {
    if(ptr = strstr(buf, "Content-length")) {
      ptr = strchr(ptr, ':');
      ptr += 1;
      while(isspace(*ptr)) ptr += 1;
      content_length = atoi(ptr);
    }
    Rio_writen(fd, buf, strlen(buf));
    printf("%s", buf);
    Rio_readlineb(rp, buf, MAXLINE);
  }
  Rio_writen(fd, buf, strlen(buf)); // 마지막에 개행 문자 보냄. (헤더의 끝을 알림)

  return content_length;
}


int parse_uri(char *uri, char *filename, char *hostname, char *port)
{
  // printf("parsing uri is %s\n", uri);
  // printf("empty? filename %s\n", filename);
  
  char *start_idx = NULL, *port_idx = NULL, *path_idx = NULL;
  
  strcpy(filename, "/");
  // printf("empty!!! filename %s\n", filename);
  if (*uri == '/') {
    uri += 1;
  }
  if (start_idx = strstr(uri, "//")) uri = start_idx + 2;

  if (port_idx = strchr(uri, ':')) {
    *port_idx = '\0';
    if (path_idx = strchr(port_idx + 1, '/')) {
      *path_idx = '\0';
      strcat(filename, path_idx + 1);
    }
    strcpy(port, port_idx + 1);
  }
  strcpy(hostname, uri);
  if (port_idx == NULL) strcpy(port, "80");
  // printf("parsing filename is %s\n", filename);
  
  return 0;

}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}
