typedef struct List{
    unsigned int item;
    struct List *next;
} List;

void addNode(List*,unsigned int);
unsigned int pop(List*);
void printList(List*);
