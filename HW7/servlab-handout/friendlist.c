/*
 * friendlist.c - [Starting code for] a web-based friend-graph manager.
 *
 * Based on:
 *  tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *      GET method to serve static and dynamic content.
 *   Tiny Web server
 *   Dave O'Hallaron
 *   Carnegie Mellon University
 */
#include "csapp.h"
#include "dictionary.h"
#include "more_string.h"

static void doit(int fd);
static dictionary_t *read_requesthdrs(rio_t *rp);
static void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);
static void clienterror(int fd, char *cause, char *errnum,
                        char *shortmsg, char *longmsg);
static void print_stringdictionary(dictionary_t *d);
static void serve_request(int fd, dictionary_t *query);
static void serve_request_friends(int fd, dictionary_t *query);
static void serve_request_befriend(int fd, dictionary_t *query);
static void serve_request_unfriend(int fd, dictionary_t *query);
static void serve_request_introduce(int fd, dictionary_t *query);

static void build_and_send_friends(int fd, dictionary_t* userFriends);

static dictionary_t *users;
static char *main_port;

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  main_port = argv[1]; // port number by user.
  users = make_dictionary(COMPARE_CASE_SENS,  free);
  listenfd = Open_listenfd(argv[1]);

  /* Don't kill the server if there's an error, because
     we want to survive errors due to a client. But we
     do want to report errors. */
  exit_on_error(0);

  /* Also, don't stop on broken connections: */
  Signal(SIGPIPE, SIG_IGN);

  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (connfd >= 0) {
      Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
                  port, MAXLINE, 0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);
      doit(connfd);
      Close(connfd);
    }
  }
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd)
{
  char buf[MAXLINE], *method, *uri, *version;
  rio_t rio;
  dictionary_t *headers, *query;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
    return;
  printf("%s", buf);

  if (!parse_request_line(buf, &method, &uri, &version)) {
    clienterror(fd, method, "400", "Bad Request",
                "Friendlist did not recognize the request");
  } else {
    if (strcasecmp(version, "HTTP/1.0")
        && strcasecmp(version, "HTTP/1.1")) {
      clienterror(fd, version, "501", "Not Implemented",
                  "Friendlist does not implement that version");
    } else if (strcasecmp(method, "GET")
               && strcasecmp(method, "POST")) {
      clienterror(fd, method, "501", "Not Implemented",
                  "Friendlist does not implement that method");
    } else {
      headers = read_requesthdrs(&rio);

      /* Parse all query arguments into a dictionary */
      query = make_dictionary(COMPARE_CASE_SENS, free);
      parse_uriquery(uri, query);
      if (!strcasecmp(method, "POST"))
        read_postquery(&rio, headers, query);

      /* For debugging, print the dictionary */
      print_stringdictionary(query);
      // printf("%s\n", uri); // "/friends?user=bob"

      /* You'll want to handle different queries here,
         but the intial implementation always returns
         nothing: */
      char **queries = split_string(uri, '?');
      // printf("%s %s\n", queries[0], queries[1]);
      dictionary_t* temp = make_dictionary(COMPARE_CASE_INSENS, free);
      parse_query(uri, temp);
      char* uriquery = (char*) dictionary_keys(temp)[0];

      if (starts_with("/friends", uriquery)){
        printf("Sucess parsing URI\n");
        serve_request_friends(fd, query);
      }else if(starts_with("/befriend", uriquery)){

      }else if(starts_with("/unfriend", uriquery)){

      }else if(starts_with("/introduce", uriquery)){

      }
      // serve_request(fd, query);

      /* Clean up */
      free(queries);
      free_dictionary(query);
      free_dictionary(headers);
    }

    /* Clean up status line */
    free(method);
    free(uri);
    free(version);
  }
}

/*
 * read_requesthdrs - read HTTP request headers
 */
dictionary_t *read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  dictionary_t *d = make_dictionary(COMPARE_CASE_INSENS, free);

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    parse_header_line(buf, d);
  }

  return d;
}

void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *dest)
{
  char *len_str, *type, *buffer;
  int len;

  len_str = dictionary_get(headers, "Content-Length");
  len = (len_str ? atoi(len_str) : 0);

  type = dictionary_get(headers, "Content-Type");

  buffer = malloc(len+1);
  Rio_readnb(rp, buffer, len);
  buffer[len] = 0;

  if (!strcasecmp(type, "application/x-www-form-urlencoded")) {
    parse_query(buffer, dest);
  }

  free(buffer);
}

static char *ok_header(size_t len, const char *content_type) {
  char *len_str, *header;

  header = append_strings("HTTP/1.0 200 OK\r\n",
                          "Server: Friendlist Web Server\r\n",
                          "Connection: close\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n",
                          "Content-type: ", content_type, "\r\n\r\n",
                          NULL);
  free(len_str);

  return header;
}

// /*
//  * serve_request - example request handler
//  */
// static void serve_request(int fd, dictionary_t *query)
// {
//   size_t len;
//   char *body, *header;
//
//   body = strdup("alice\nbob");
//
//   len = strlen(body);
//
//   /* Send response headers to client */
//   header = ok_header(len, "text/html; charset=utf-8");
//   Rio_writen(fd, header, strlen(header));
//   printf("Response headers:\n");
//   printf("%s", header);
//
//   free(header);
//
//   /* Send response body to client */
//   Rio_writen(fd, body, len);
//
//   free(body);
// }

/*
 * Adds each user in ‹friends› as a friend of ‹user›, which implies
 * adding ‹user› as a friend of each user in ‹friends›.
 *
 * The ‹friends› list can be a single user or multiple newline-separated
 * user names, and ‹friends› can optionally end with a newline character.
 *
 * /befriend?user=‹user›&friends=‹friends›
 */
static void serve_request_friends(int fd, dictionary_t *query)
{
  char *user = dictionary_get(query, "user");
  dictionary_t* friend_of_users = dictionary_get(users, user);
  if (friend_of_users == NULL){
    dictionary_t* friends = make_dictionary(COMPARE_CASE_INSENS, free);
    dictionary_set(users, user, friends);
    friend_of_users = dictionary_get(users, user);
  }
  size_t len;
  char *body, *header;

  body = join_strings(dictionary_keys(friend_of_users), '\n');
  printf("Body:\n");
  printf("%s", body);

  len = strlen(body);

  /* Send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  free(body);
}


/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum,
		 char *shortmsg, char *longmsg)
{
  size_t len;
  char *header, *body, *len_str;

  body = append_strings("<html><title>Friendlist Error</title>",
                        "<body bgcolor=""ffffff"">\r\n",
                        errnum, " ", shortmsg,
                        "<p>", longmsg, ": ", cause,
                        "<hr><em>Friendlist Server</em>\r\n",
                        NULL);
  len = strlen(body);

  /* Print the HTTP response */
  header = append_strings("HTTP/1.0 ", errnum, " ", shortmsg, "\r\n",
                          "Content-type: text/html; charset=utf-8\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n\r\n",
                          NULL);
  free(len_str);

  Rio_writen(fd, header, strlen(header));
  Rio_writen(fd, body, len);

  free(header);
  free(body);
}

static void print_stringdictionary(dictionary_t *d)
{
  int i, count;

  count = dictionary_count(d);
  for (i = 0; i < count; i++) {
    printf("%s=%s\n",
           dictionary_key(d, i),
           (const char *)dictionary_value(d, i));
  }
  printf("\n");
}
