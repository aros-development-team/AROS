#ifndef ACDR_PATH_H
#define ACDR_PATH_H

typedef struct path_node {
  uint32_t references;
  char *name;
  struct path_node *next;
} t_path_node;

t_path_list Append_Path_List(t_path_list, char *);
t_path_list Copy_Path_List (t_path_list, int);
void Free_Path_List(t_path_list);
t_bool Path_Name_From_Path_List(t_path_list, char*, int);

#endif /* ACDR_PATH_H */
