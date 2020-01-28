#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/list.h"

void addNode(List **list, int item)
{
    if (list == NULL || *list == NULL) {
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
    if(list == NULL || *list == NULL)
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
    if (list == NULL || *list == NULL) {
        *list = malloc(sizeof(string_List));
        (*list)->item = (char*) malloc(strlen(item)+1);
        strcpy((*list)->item, item);
        (*list)->next = NULL;
    } else {
        string_List *newlist = malloc(sizeof(string_List));
        newlist->item = item;
        newlist->item = (char*) malloc(strlen(item)+1);
        strcpy(newlist->item, item);
        newlist->next = *list;
        *list = newlist;
    }
}

char *pop_string(string_List **list)
{
    string_List *temp;
    char *result;
    if(list == NULL || *list == NULL)
        return NULL;
    temp = *list;
    result = (*list)->item;
    *list = (*list)->next;
    free(temp);
    return result;
}

void destroy_stringList(string_List *list)
{
    while (list != NULL) {
        free(pop_string(&list));
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

char *pop_minimum_string(string_List** list)
{
    if (list == NULL || *list == NULL)
        return NULL;
    char *minItem = (*list)->item;
    string_List *minNode = *list;
    string_List *temp = *list;
    while (temp != NULL)
    {
        if (strcmp(temp->item, minItem) < 0) {
            minItem = temp->item;
            minNode = temp;
        }
        temp = temp->next;
    }
    if (strcmp((*list)->item, minItem) == 0) {
        *list = (*list)->next;
        free(minNode);
        return minItem;
    } else {
        temp = *list;
        while (temp->next != NULL)
        {
            if (temp->next == minNode) {
                temp->next = minNode->next;
                free(minNode);
                return minItem;
            }
            temp = temp->next;
        }
    }
    return NULL;
}

char *pop_last_string(string_List** list)
{
    if (list == NULL || *list == NULL)
        return NULL;
    char *result;
    if ((*list)->next == NULL) {
        result = (*list)->item;
        free(*list);
        *list = NULL;
        return result;
    }
    string_List *temp = *list;
    while (temp->next->next != NULL)
    {
        temp = temp->next;
    }
    result = temp->next->item;
    free(temp->next);
    temp->next = NULL;
    return result;
}

int getLength(string_List *list)
{
    int counter = 0;
    string_List *temp = list;
    while (temp != NULL)
    {
        counter++;
        temp = temp->next;
    }
    return counter;
}

char *get_stringNode(string_List** list,int n)
{
	if(list == NULL || *list == NULL)
		return NULL;
	int		i = 1;
	string_List	*temp = *list;
	while(temp != NULL && i<= n)
	{
		if(i == n)
			return temp->item;

		temp = temp->next;
		i++;
	}

	return NULL;
}