#include <stdio.h>
#include <stdlib.h>
#include "../include/list.h"

void addNode(List **list, int item)
{
    if (*list == NULL) {
        *list = malloc(sizeof(List));
        (*list)->item = item;
        (*list)->next = NULL;
    } else {
        List *newlist = malloc(sizeof(List));
        newlist->item = item;
        newlist->next = *list;
        *list = newlist;
    }
}

int pop(List **list)
{
    List *temp;
    int result;
    if(*list == NULL)
        return 0;
    temp = *list;
    result = (*list)->item;
    *list = (*list)->next;
    free(temp);
    return result;
}

void destroyList(List *list)
{
    while (list != NULL) {
        pop(&list);
    }
}

void printList(List *list)
{
    List *temp = list;
    while (temp != NULL) {
        printf("(%d)->", temp->item);
        temp=temp->next;
    }
    printf("\n");
}

void add_stringNode(string_List **list, char *item)
{
    if (*list == NULL) {
        *list = malloc(sizeof(string_List));
        (*list)->item = item;
        (*list)->next = NULL;
    } else {
        string_List *newlist = malloc(sizeof(string_List));
        newlist->item = item;
        newlist->next = *list;
        *list = newlist;
    }
}

char *pop_string(string_List **list)
{
    string_List *temp;
    char *result;
    if(*list == NULL)
        return 0;
    temp = *list;
    result = (*list)->item;
    *list = (*list)->next;
    free(temp);
    return result;
}

void destroy_stringList(string_List *list)
{
    while (list != NULL) {
        pop_string(&list);
    }
}

void print_stringList(string_List *list)
{
    string_List *temp = list;
    while (temp != NULL) {
        printf("(%s)->", temp->item);
        temp=temp->next;
    }
    printf("\n");
}

