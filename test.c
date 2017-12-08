#include <stdio.h>

int main(){
  char* charpointer = NULL;
  double* doublepointer = NULL;
  int* intpointer = NULL;

  printf("Size of char pointer: %lu\n", sizeof(charpointer));
  printf("Size of double pointer: %lu\n", sizeof(doublepointer));
  printf("Size of int pointer: %lu\n", sizeof(intpointer));

  return 1;
}
