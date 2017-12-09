/* Missing friend
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

#define CRLF ("\r\n\r\n")

static void *doit(void* fd_p);
static dictionary_t *read_requesthdrs(rio_t *rp);
static void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);
static void clienterror(int fd, char *cause, char *errnum,
                        char *shortmsg, char *longmsg);
static void print_stringdictionary(dictionary_t *d);
// static void serve_request(int fd, dictionary_t *query);
static void serve_request_friends(int fd, dictionary_t *query);
static void serve_request_befriend(int fd, dictionary_t *query);
static void serve_request_unfriend(int fd, dictionary_t *query);
static void serve_request_introduce(int fd, dictionary_t *query);

static void handle_user_with_new_friend(dictionary_t* new_friends, char* user, char* friend);
static void handle_new_friend_with_user(dictionary_t* friend_of_adding_friend, char* user, char* friend);
static void add_friend_to_dict(char* user, char** adding_friends);
static void remove_friend_from_dict(dictionary_t* friend_of_users, char* user, char** removing_friends);
static void add_friend_to_dict_introduce(char* user, char** adding_friends);

// static char* server_portno;
static dictionary_t *users;
static pthread_mutex_t lock;

typedef int bool;
#define true 1
#define false 0


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

  pthread_mutex_init(&lock, NULL);
  users = make_dictionary(COMPARE_CASE_SENS,  free);
  listenfd = Open_listenfd(argv[1]);
  // server_portno = argv[1];

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

      pthread_t th;
      int* ptr = malloc(sizeof(int));
      *ptr = connfd;
      Pthread_create(&th, NULL, doit, ptr);
      Pthread_detach(th);
    }
  }
}
/*
 * doit - handle one HTTP request/response transaction
 */
void *doit(void* fd_p)
{
  int fd = *(int *)fd_p;
  char buf[MAXLINE], *method, *uri, *version;
  rio_t rio;
  dictionary_t *headers, *query;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
    return NULL;
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
        printf("Sucess parsing URI\n");
        serve_request_befriend(fd, query);
      }else if(starts_with("/unfriend", uriquery)){
        serve_request_unfriend(fd, query);
      }else if(starts_with("/introduce", uriquery)){
        serve_request_introduce(fd, query);
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
    Close(fd);
  }
  return NULL;
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
  pthread_mutex_lock(&lock);
  printf("Printing the query:\n");

  // print_stringdictionary(query);
  char *user = dictionary_get(query, "user");
  printf("User: %s\n", user);

  dictionary_t* friend_of_users = dictionary_get(users, user);
  if (friend_of_users == NULL){
    dictionary_set(users, user, make_dictionary(COMPARE_CASE_SENS, free));
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
  pthread_mutex_unlock(&lock);
  free(body);
}

static void handle_user_with_new_friend(dictionary_t* new_friends, char* user, char* friend){
  if (new_friends == NULL){
    new_friends = make_dictionary(COMPARE_CASE_SENS, free);
    dictionary_set(users, user, make_dictionary(COMPARE_CASE_SENS, free));
  }

  if (dictionary_get(new_friends, friend) == NULL){
    dictionary_set(new_friends, friend, NULL);
  }
}

static void handle_new_friend_with_user(dictionary_t* friend_of_adding_friend, char* user, char* friend){
  if (friend_of_adding_friend == NULL){
    friend_of_adding_friend = make_dictionary(COMPARE_CASE_SENS, free);
    dictionary_set(users, friend, friend_of_adding_friend);
  }

  if(dictionary_get(friend_of_adding_friend, user)==NULL){
    dictionary_set(friend_of_adding_friend, user, NULL);
  }
}

static void add_friend_to_dict(char* user, char** adding_friends){
  size_t i, total;
  // adding user in <freinds>
  for (i = total = 0; adding_friends[i] != NULL; i++){
    if(strcmp(user, adding_friends[i]) != 0){
      // printf("adding_friends:%s \n", adding_friends[i]);
      dictionary_t* new_friends = dictionary_get(users, user);
      handle_user_with_new_friend(new_friends, user, adding_friends[i]);

      dictionary_t* friend_of_adding_friend = dictionary_get(users, adding_friends[i]);
      handle_new_friend_with_user(friend_of_adding_friend, user, adding_friends[i]);
    }
  }
}

static void serve_request_befriend(int fd, dictionary_t *query){
  pthread_mutex_lock(&lock);
  char *user = dictionary_get(query, "user");
  printf("User from be-friend: %s\n", user);
  char** adding_friends;

  dictionary_t* friend_of_users = dictionary_get(users, user);
  if (friend_of_users == NULL){
    dictionary_set(users, user, make_dictionary(COMPARE_CASE_SENS, free));
  }

  adding_friends = split_string(dictionary_get(query, "friends"), '\n');
  add_friend_to_dict(user, adding_friends);
  friend_of_users = dictionary_get(users, user);

  // Outputting the result
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
  pthread_mutex_unlock(&lock);
  free(body);
}

static void remove_friend_from_dict(dictionary_t* friend_of_users, char* user, char** removing_friends){
  size_t i, total;
  // removing user in <freinds>
  for (i = total = 0; removing_friends[i] != NULL; i++){;
    dictionary_remove(friend_of_users, removing_friends[i]);
    dictionary_t* removing_friends_dict = dictionary_get(users, removing_friends[i]);

    // Deleting a friend
    if (removing_friends_dict != NULL){
      dictionary_remove(removing_friends_dict, user);
    }
  }
}

/*
 * /unfriend?user=‹user›&friends=‹friends›
 */
static void serve_request_unfriend(int fd, dictionary_t *query){
  pthread_mutex_lock(&lock);
  // parse_query(fd, query);
  char *user = dictionary_get(query, "user");
  char** removing_friends;
  // printf("User from be-friend: %s\n", user);
  dictionary_t* friend_of_users = dictionary_get(users, user);

  if (friend_of_users == NULL){
    dictionary_set(users, user, make_dictionary(COMPARE_CASE_SENS, free));
  }

  removing_friends = split_string(dictionary_get(query, "friends"), '\n');
  remove_friend_from_dict(friend_of_users, user, removing_friends);

  friend_of_users = dictionary_get(users, user);

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
  pthread_mutex_unlock(&lock);
  free(body);
}

static void add_friend_to_dict_introduce(char* user, char** adding_friends){
  size_t i, total;
  // adding user in <freinds>
  for (i = total = 0; adding_friends[i] != NULL; i++){
    if(strcmp(user, adding_friends[i]) != 0){
      printf("adding_friends:%s \n", adding_friends[i]);
      dictionary_t* new_friends = dictionary_get(users, user);
      handle_user_with_new_friend(new_friends, user, adding_friends[i]);

      dictionary_t* friend_of_adding_friend = dictionary_get(users, adding_friends[i]);
      handle_new_friend_with_user(friend_of_adding_friend, adding_friends[i], user);
    }
    free(adding_friends[i]);
  }
}


static void serve_request_introduce(int fd, dictionary_t *query){
  char* host = dictionary_get(query,"host");
  char *portno = dictionary_get(query,"port");

  char *user = dictionary_get(query,"user");
  char *friend = dictionary_get(query,"friend");

  rio_t rio;
  char *buf;

  // From TCP Slide
  struct addrinfo hints;
  struct addrinfo *addrs;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; /* Request IPv4 */
  hints.ai_socktype = SOCK_STREAM; /* Accept TCP connection */
  // hints.ai_flags = AI_PASSIVE; /* ... on any IP address */
  Getaddrinfo(host, portno, &hints, &addrs);

  int s = Socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
  Connect(s, addrs->ai_addr, addrs->ai_addrlen);
  Freeaddrinfo(addrs);

  buf = append_strings("GET /friends?user=", friend," HTTP/1.1\r\n",
                        CRLF, NULL);

  Rio_writen(s, buf, strlen(buf)); // Send buffer
  Shutdown(s, SHUT_WR); // Leave only reading-half open, because we won't be writing anymore
  Rio_readinitb(&rio, s);  // Receive result from host

  dictionary_t *result_from_server = read_requesthdrs(&rio);
  size_t content_length = atoi(dictionary_get(result_from_server, "Content-length"));

  char content_buf[content_length];
  Rio_readnb(&rio, content_buf, content_length);
  content_buf[content_length] = 0;

  pthread_mutex_lock(&lock);

  dictionary_t *friend_of_users = dictionary_get(users,user);
  if(friend_of_users == NULL){
    dictionary_set(users, user, make_dictionary(COMPARE_CASE_SENS,NULL));
  }

  char** adding_friends = split_string(content_buf, '\n');
  add_friend_to_dict_introduce(user, adding_friends);
  free(adding_friends);

  char *body = join_strings(dictionary_keys(friend_of_users),'\n');
  //Respond back to client

  // char *header;
  printf("Body:\n");
  printf("%s", body);

  size_t len;
  len = strlen(body);

  /* Send response headers to client */
  char* header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  free(body);
  pthread_mutex_unlock(&lock);
  Close(s);
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
