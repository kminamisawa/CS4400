/* This is the main file for the `whoosh` interpreter and the part
   that you modify. */

#include <stdlib.h>
#include <stdio.h>
#include "csapp.h"
#include "ast.h"
#include "fail.h"
#include <string.h>

#define READ  (0)
#define WRITE (1)

static void run_script(script *scr);

static void run_group(script_group *group);

static void run_and_group(script_group *group);

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
            int mode = scr->groups[i].mode;
            if(mode == GROUP_SINGLE){
              run_group(&scr->groups[i]);
            }else if (mode == GROUP_OR){
              //run_group(&scr->groups[i]);
            }else if (mode == GROUP_AND){
              run_and_group(&scr->groups[i]);
            }else{
              break;
            }
        }
        // /* You'll have to make run_script do better than this */
        // fail("only 1 group supported");
    }
}

static void run_group(script_group *group) {
    script_var *output = group->commands->output_to;
    script_var *input = group->commands->input_from;

    int i, j;
    for (i = 0; i < group->repeats; i++) {
        for (j = 0; j < group->num_commands; j++) {
            int pipe_child2parent[2];
            int pipe_parent2child[2];

            Pipe(pipe_child2parent);
            Pipe(pipe_parent2child);

            pid_t pid = fork();
            if (pid == 0) {

                if (output != NULL) {
                    // 子→親への入力を標準出力に割り当て
                    Dup2(pipe_child2parent[WRITE], 1); //KEEP THIS

                    // 子プロセスの場合は、親→子への書き込みはありえないのでcloseする
                    Close(pipe_parent2child[WRITE]);

                    // 子プロセスの場合は、子→親の読み込みはありえないのでcloseする
                    Close(pipe_parent2child[READ]);

//                    close(pipe_child2parent[WRITE]);
                    Close(pipe_child2parent[READ]);
                }


                if (input != NULL) {
//                    dup2(pipe_child2parent[WRITE], 1);
//                    printf("rrrr");

                    write_var_to(pipe_child2parent[WRITE], input);
                    // 親→子への出力を標準入力として割り当て
                    Dup2(pipe_child2parent[READ], 0);

                    Close(pipe_parent2child[WRITE]);
                    Close(pipe_parent2child[READ]);

                    Close(pipe_child2parent[WRITE]);
                    Close(pipe_child2parent[READ]);
                }

                run_command(&group->commands[j]);
                if (group->commands[j].pid_to != 0) {
                    set_var(group->commands[j].pid_to, pid);
                }
            } else {
                int status;

                int status_val = 0;

                Close(pipe_parent2child[WRITE]);

                Close(pipe_child2parent[WRITE]);
                Close(pipe_parent2child[READ]);

                Waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    status_val = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    status_val = WTERMSIG(status) * -1;
                } else if (WIFSTOPPED(status)) {
                    status_val = WSTOPSIG(status);
                }

                if (output != NULL) {
                    if (status != 0 && output != 0) {
                        set_var(output, status_val);
                    }else
                        read_to_var(pipe_child2parent[READ], output);
                }
            }
        }
    }
}

static void run_and_group(script_group *group) {
  // script_var *output = group->commands->output_to;
  // script_var *input = group->commands->input_from;

  int i, j;
  for (i = 0; i < group->repeats; i++) {
    script_var *output = group->commands->output_to;
    script_var *input = group->commands->input_from;
      for (j = 0; j < group->num_commands; j++) {
          int pipe_child2parent[2];
          int pipe_parent2child[2];

          Pipe(pipe_child2parent);
          Pipe(pipe_parent2child);

          pid_t pid = fork();
          if (pid == 0) {
              if (output != NULL) {
                  // 子→親への入力を標準出力に割り当て
                  Dup2(pipe_child2parent[WRITE], 1); //KEEP THIS

                  // 子プロセスの場合は、親→子への書き込みはありえないのでcloseする
                  Close(pipe_parent2child[WRITE]);

                  // 子プロセスの場合は、子→親の読み込みはありえないのでcloseする
                  Close(pipe_parent2child[READ]);

//                    close(pipe_child2parent[WRITE]);
                  Close(pipe_child2parent[READ]);
              }


              if (input != NULL) {
//                    dup2(pipe_child2parent[WRITE], 1);
//                    printf("rrrr");

                  write_var_to(pipe_child2parent[WRITE], input);
                  // 親→子への出力を標準入力として割り当て
                  Dup2(pipe_child2parent[READ], 0);

                  Close(pipe_parent2child[WRITE]);
                  Close(pipe_parent2child[READ]);

                  Close(pipe_child2parent[WRITE]);
                  Close(pipe_child2parent[READ]);
              }

              run_command(&group->commands[j]);
              if (group->commands[j].pid_to != 0) {
                  set_var(group->commands[j].pid_to, pid);
              }
          } else {
              int status;

              int status_val = 0;

              Close(pipe_parent2child[WRITE]);

              Close(pipe_child2parent[WRITE]);
              Close(pipe_parent2child[READ]);

              Waitpid(pid, &status, 0);
              if (WIFEXITED(status)) {
                  status_val = WEXITSTATUS(status);
              } else if (WIFSIGNALED(status)) {
                  status_val = WTERMSIG(status) * -1;
              } else if (WIFSTOPPED(status)) {
                  status_val = WSTOPSIG(status);
              }

              if (output != NULL) {
                  if (status != 0 && output != 0) {
                      set_var(output, status_val);
                  }else
                      read_to_var(pipe_child2parent[READ], output);
              }
          }
      }
  }
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
            argv[i + 1] = command->arguments[i].u.literal;
        else
            argv[i + 1] = command->arguments[i].u.var->value;
    }

    argv[command->num_arguments + 1] = NULL;

    Execve(argv[0], (char *const *) argv, environ);

    free(argv);
}

/* You'll likely want to use this set_var function for converting a
   numeric value to a string and installing it as a variable's
   value: */
static void set_var(script_var *var, int new_value) {
    char buffer[32];
    free((void *) var->value);
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
            if (amt && (buffer[amt - 1] == '\n'))
                amt--;
            buffer[amt] = 0;
            free((void *) var->value);
            var->value = strdup(buffer);
            return;
        }
        amt += got;
        if (amt > (size - 1))
            app_error("received too much output");
    }
}
