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
//grw - flag to match as prefix substring
int prefix = 0;
//grw - insensitive case match flag
int icase  = 0;

//grw - flag to show only directories

int walk(const char* directory, const char* name, counter_t *counter) {
  entry_t *head = NULL, *current, *iter;
  size_t size   = 0, index;
  int    dirlen = 0;
  int    found  = 0;

  struct dirent *file_dirent;
  DIR *dir_handle;

  char *full_path;

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
    found = 0;
    if (prefix) {
      if (icase) {
        found = !strncasecmp(head->name, name, strlen(name));
      } else {
        found = !strncmp(head->name, name, strlen(name));
      }
    } else if (icase) {
      found = !strcasecmp(head->name, name);
    } else {
      found = !strcmp(head->name, name);
    }

    if (found) {
      printf("%s/%s\n", directory, head->name);
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

      walk(full_path, name, counter);
      free(full_path);
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

void usage(void) {
  fprintf(stderr, "Usage: find [-h|-i] name [directory, default = current]\n");
  fprintf(stderr, " Show find entries matching name in a directory and its sub-directories\n");
  fprintf(stderr, "  -h = show hidden items\n  -i = case insensitive match.\n  -p = match name as prefix\n");
}

int main(int argc, char *argv[]) {
  //char* directory = argc > 1 ? argv[1] : ".";
  counter_t counter = {0, 0};
  char *name = NULL;
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
            } else if (c == 'i') {
              icase = 1;
            } else if (c == 'p') {
              prefix = 1;
            } else {
              usage();
              return -1;
            } // if-else c
            j++;
        } // while
      } else {
        //grw - first argument without dash is name
        if (!name) {
          name = argv[i];
        } else if (!directory) {
          //grw - second argument without dash is optional directory
          directory = argv[i];
        } else {
          //grw - third argument without dash is syntax error
          usage();
          return -1;
        } //if-else name
      } //if-else argv[i][0]
    } //for
  } //if argc > 1

  //grw - if no name, syntax error
  //grw - if no directory given, use current directory
  if (!name) {
    usage();
    return -1;
  } else if (!directory) {
    directory = ".";
  }

  printf("Searching %s\n", directory);

  walk(directory, name, &counter);

  printf("\nsearched %d directories, %d files\n",
    counter.dirs ? counter.dirs - 1 : 0, counter.files);
  return 0;
}
