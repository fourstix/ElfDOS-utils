/*
 *  Elf-DOS Tree utilitiy
 *  Copyright 2026 by Gaston Williams
 *
 *  Based on original code
 *  Copyright (c) 2016-2026 by Kevin Newton
 *  https://github.com/kddnewton/tree
 *
 * Changes:
 *   grw - Updated to use ELFC dirent library
 *   grw - Converted symbols to ASCII characters
 *   grw - Added logic to not duplicate path separator
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#define DE_ISHIDDEN(e)   ((e)->d_attr & _A_HIDDEN)

typedef struct counter {
  size_t dirs;
  size_t files;
} counter_t;

typedef struct entry {
  char *name;
  int is_dir;
  struct entry *next;
} entry_t;

//grw - flag to show hidden entries
int show = 0;
//grw - flag to show only directories
int dironly = 0;

//grw - flag to show only directories

int walk(const char* directory, const char* prefix, counter_t *counter) {
  entry_t *head = NULL, *current, *iter;
  size_t size   = 0, index;
  int    dirlen = 0;

  struct dirent *file_dirent;
  DIR *dir_handle;

  char *full_path, *segment, *pointer, *next_prefix;

  dir_handle = opendir(directory);
  // if we can't open the direcctory try again before giving up
  if (!dir_handle) {
    //grw - try opening directory off root without leading dot
    if (directory[0] == '.' && directory[1] == '/') {
      dir_handle = opendir(directory + 1);
    }
    if (!dir_handle) {
      fprintf(stderr, "Cannot open directory \"%s\"\n", directory);
      return -1;
    }
  }

  counter->dirs++;

  while ((file_dirent = readdir(dir_handle)) != NULL) {
    if (file_dirent->d_name[0] == '.') {
      continue;
    }

    //grw - skip hidden entries
    if (!show && DE_ISHIDDEN(file_dirent)){
      continue;
    }

    current = malloc(sizeof(entry_t));
    current->name = strcpy(malloc(strlen(file_dirent->d_name) + 1), file_dirent->d_name);
    //current->is_dir = file_dirent->d_type == DT_DIR;
    current->is_dir = DE_ISDIR(file_dirent);
    current->next = NULL;

    if (head == NULL) {
      head = current;
    } else if (strcmp(current->name, head->name) < 0) {
      current->next = head;
      head = current;
    } else {
      for (iter = head; iter->next && strcmp(current->name, iter->next->name) > 0; iter = iter->next);

      current->next = iter->next;
      iter->next = current;
    }

    size++;
  }

  closedir(dir_handle);
  if (!head) {
    return 0;
  }

  for (index = 0; index < size; index++) {
    if (index == size - 1) {
      pointer = "+-- ";
      segment = "    ";
    } else {
      pointer = "|-- ";
      segment = "│   ";
    }

    if (!dironly || head->is_dir) {
      printf("%s%s%s\n", prefix, pointer, head->name);
    }

    if (head->is_dir) {
      dirlen = strlen(directory);
      full_path = malloc(dirlen + strlen(head->name) + 2);

      //grw - if a directory ends in slash, don't double it
      if (directory[dirlen - 1] == '/') {
        sprintf(full_path, "%s%s", directory, head->name);
      } else {
        sprintf(full_path, "%s/%s", directory, head->name);
      }

      next_prefix = malloc(strlen(prefix) + strlen(segment) + 1);
      sprintf(next_prefix, "%s%s", prefix, segment);

      walk(full_path, next_prefix, counter);
      free(full_path);
      free(next_prefix);
    } else {
      counter->files++;
    }

    current = head;
    head = head->next;

    free(current->name);
    free(current);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  //char* directory = argc > 1 ? argv[1] : ".";
  counter_t counter = {0, 0};
  char *directory = NULL;
  int  i, j;
  char c;

  if (argc > 1) {
    for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-') {
        j = 1;
        while(c = argv[i][j]) {
            if (c == 'h') {
              show = 1;
            } else if (c == 'd') {
              dironly = 1;
            } else {
              fprintf(stderr, "Usage: tree [-d|-h] [directory, default = current]\n");
              fprintf(stderr, " Show contents of a directory and its sub-directories\n");
              fprintf(stderr, " -d = show only directories.\n -h = show hidden items\n");
              return -1;
            } // if-else c
            j++;
        } // while
      } else {
        directory = argv[i];
      } //if-else argv[i][0]
    } //for
  } //if argc > 1

  //grw - if no directory give use current directory
  if (!directory) {
    directory = ".";
  }

  printf("%s\n", directory);

  walk(directory, "", &counter);

  printf("\n%d directories, %d files\n",
    counter.dirs ? counter.dirs - 1 : 0, counter.files);
  return 0;
}
