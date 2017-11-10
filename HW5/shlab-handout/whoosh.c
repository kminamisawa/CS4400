/* This is the main file for the `whoosh` interpreter and the part
   that you modify. */

#include <stdlib.h>
#include <stdio.h>
#include "csapp.h"
#include "ast.h"
#include "fail.h"

#define READ  (0)
#define WRITE (1)

static void run_script(script *scr);
static void run_group(script_group *group);
static void run_command(script_command *command);
static void set_var(script_var *var, int new_value);
static void read_to_var(int fd, script_var *var);
static void write_var_to(int fd, script_var *var);

/* You probably shouldn't change main at all. */

int main(int argc, char **argv) {
  script *scr;

  if ((argc != 1) && (argc != 2)) {
    fprintf(stderr, "usage: %s [<script-file>]\n", argv[0]);
    exit(1);
  }

  scr = parse_script_file((argc > 1) ? argv[1] : NULL);

  run_script(scr);

  return 0;
}

// /* A parsed whoosh script: */
// typedef struct {
//   int num_groups;                 /* length of the `groups` array */
//   struct script_group *groups;    /* sequence of groups to run */
// } script;
//
// /* A group within a script: */
// typedef struct script_group {
//   int mode;                        /* GROUP_SINGLE, GROUP_AND, or GROUP_OR */
//   int repeats;                     /* number of copies of the commands to run */
//   int num_commands;                /* length of the `commands` array; GROUP_SINGLE => 1 */
//   struct script_command *commands; /* commands to run in the group */
// } script_group;
//
// /* Values for `mode`: */
// #define GROUP_SINGLE 0
// #define GROUP_AND    1
// #define GROUP_OR     2

static void run_script(script *scr) {
  if (scr->num_groups == 1) {
    //run_group(&scr->groups[0]);
    run_group(&scr->groups[0]);
  } else {
    int i;
    for (i = 0; i < scr->num_groups; i++) {
      run_group(&scr->groups[i]);
    }
    // /* You'll have to make run_script do better than this */
    // fail("only 1 group supported");
  }
}

static void run_group(script_group *group) {
  /* You'll have to make run_group do better than this, too */
  // if (group->repeats != 1)
  //   fail("only repeat 1 supported");

  script_var* output = group->commands->output_to;
  script_var* input = group->commands->input_from;

  if(output != NULL){
    int i,j;
    for (i = 0; i < group->repeats; i++){
      for (j = 0; j < group->num_commands; j++){
        //  pid_t pid = fork();
        int pipe_child2parent[2];
        int pipe_parent2child[2];

        Pipe(pipe_child2parent);
        Pipe(pipe_parent2child);

        pid_t pid = fork();
        if(pid == 0){
          Close(pipe_parent2child[WRITE]);
          Close(pipe_child2parent[READ]);

          // dup2(pipe_parent2child[READ], 0);
          // dup2(pipe_child2parent[WRITE], 1);
          // write_var_to(pipe_parent2child[WRITE], input);
          //write_var_to(fds[WRITE], input);
          dup2(pipe_parent2child[READ], 0);
          Setpgid(0,0);
          run_command(&group->commands[j]);
          if(group->commands[j].pid_to != 0)
            set_var(group->commands[j].pid_to, pid);
          dup2(pipe_child2parent[WRITE], 1);
          write_var_to(pipe_parent2child[WRITE], input);
        }else{
            // Close(pipe_parent2child[READ]);
            // Close(pipe_child2parent[WRITE]);
          //  set_var(group->commands[j].pid_to, 0);
            int status;
            int status_val = 0;
            //write_var_to(pipe_parent2child[WRITE], input);

            Waitpid(pid, &status, 0);
            Close(pipe_parent2child[READ]);
            Close(pipe_child2parent[WRITE]);

            read_to_var(pipe_child2parent[READ], output);
            // write_var_to(pipe_parent2child[WRITE], input);
            //write_var_to()

            if(WIFEXITED(status)){
  					status_val = WEXITSTATUS(status);
    				} else if(WIFSIGNALED(status)){
    					status_val = WTERMSIG(status)*-1;
    				} else if(WIFSTOPPED(status)){
    					status_val = WSTOPSIG(status);
    				}

            //read_to_var(pipe_parent2child[READ], pid);
        }
      }
    }
  }else{
    int i;
    for (i = 0; i < group->repeats; i++){
      pid_t pid = fork();
      if(pid == 0){
        Setpgid(0,0);
        run_command(&group->commands[0]);
      }else{
          int status;
          Waitpid(pid, &status, 0);
      }
    }
  }





  // for (j = 0; j < group->num_commands; j++) {
  //   run_command(&group->commands[j]);
  // }
  //
  // int j;
  // for (j = 0; j < group->num_commands; j++) {
  //   run_command(&group->commands[j]);
  // }
  // if (group->num_commands == 1) {
  //   int i;
  //   for (i = 0; i < group->num_commands; i++) {
  //     pid_t pid = fork();
  //     if(pid == 0){
  //       run_command(&group->commands[i]);
  //     }else{
  //       int itpr;
  //       Waitpid(pid, &itpr, 0);
  //     }
  //   }
  // } else {
  //   /* And here */
  //   fail("only 1 command supported");
  // }
}

/* This run_command function is a good start, but note that it runs
   the command as a replacement for the `whoosh` script, instead of
   creating a new process. */

static void run_command(script_command *command) {
  const char **argv;
  int i;

  // if (command->pid_to != NULL)
  //   fail("setting process ID variable not supported");
  // if (command->input_from != NULL)
  //   fail("input from variable not supported");
  // if (command->output_to != NULL)
  //   fail("output to variable not supported");

  argv = malloc(sizeof(char *) * (command->num_arguments + 2));
  argv[0] = command->program;

  for (i = 0; i < command->num_arguments; i++) {
    if (command->arguments[i].kind == ARGUMENT_LITERAL)
      argv[i+1] = command->arguments[i].u.literal;
    else
      argv[i+1] = command->arguments[i].u.var->value;
  }

  argv[command->num_arguments + 1] = NULL;

  Execve(argv[0], (char * const *)argv, environ);

  free(argv);
}

/* You'll likely want to use this set_var function for converting a
   numeric value to a string and installing it as a variable's
   value: */
static void set_var(script_var *var, int new_value) {
  char buffer[32];
  free((void*)var->value);
  snprintf(buffer, sizeof(buffer), "%d", new_value);
  var->value = strdup(buffer);
}

/* You'll likely want to use this write_var_to function for writing a
   variable's value to a pipe: */
static void write_var_to(int fd, script_var *var) {
  size_t len = strlen(var->value);
  ssize_t wrote = Write(fd, var->value, len);
  wrote += Write(fd, "\n", 1);
  if (wrote != len + 1)
    app_error("didn't write all expected bytes");
}

/* You'll likely want to use this write_var_to function for reading a
   pipe's content into a variable: */
static void read_to_var(int fd, script_var *var) {
  size_t size = 4097, amt = 0;
  char buffer[size];
  ssize_t got;

  while (1) {
    got = Read(fd, buffer + amt, size - amt);
    if (!got) {
      if (amt && (buffer[amt-1] == '\n'))
        amt--;
      buffer[amt] = 0;
      free((void*)var->value);
      var->value = strdup(buffer);
      return;
    }
    amt += got;
    if (amt > (size - 1))
      app_error("received too much output");
  }
}
