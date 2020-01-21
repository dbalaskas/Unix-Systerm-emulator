typedef struct List{
    int item;
    struct List *next;
} List;

void addNode(List*,int);
int pop(List*);
void printList(List*);
