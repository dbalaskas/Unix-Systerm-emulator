#include <stdio.h>
#include <stdlib.h>
#include "../include/list.h"

void addNode(List *list,unsigned int blockNum)
{
    if (list == NULL) {
        list = malloc(sizeof(List));
        list->item = blockNum;
        list->next = NULL;
    } else {
        List *newlist = malloc(sizeof(List));
        newlist->item = blockNum;
        newlist = list;
        list = newlist;
    }
}

unsigned int pop(List* list)
{
    List *temp;
    unsigned int result;
    if(list == NULL)
        return 0;
    temp = list;
    result = list->item;
    list = list->next;
    free(temp);
    return result;
}

void printList(List* list)
{
    List *temp = list;
    while (temp != NULL) {
        printf("(%d) -> ", temp->item);
        temp=temp->next;
    }
    printf("\n");
}
