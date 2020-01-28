typedef struct List{
    int item;
    struct List *next;
} List;

void addNode(List**,int);
int pop(List**);
void printList(List*);
void destroyList(List*);

typedef struct string_List{
    char *item;
    struct string_List *next;
} string_List;

void add_stringNode(string_List**,char*);
char *pop_string(string_List**);
void print_stringList(string_List*);
void destroy_stringList(string_List*);
//----------------------------------------
char *pop_minimum_string(string_List**);
char *pop_last_string(string_List**);
int getLength(string_List*);
char *get_stringNode(string_List**,int);
