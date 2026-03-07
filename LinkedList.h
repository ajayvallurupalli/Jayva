#ifndef LinkedListIncluded
#define LinkedListIncluded

typedef struct Cons {
        void*        val;
        struct Cons* next;
} Cons;

typedef struct {
        struct Cons* list;
} LinkedList;

LinkedList mkLinkedList();
void freeLinkedListInners(LinkedList* list);

LinkedList consLinkedList(LinkedList list, void* val);
void consLinkedListByRef(LinkedList* list, void* val);

void* getElementOfLinkedList(LinkedList* list, void* context, int (*pred) (void*, void*));
void mapLinkedList(LinkedList* list, void (*map) (void*));
void forEachLinkedList(const LinkedList* list, void (*map) (const void*));

void reverseLinkedList(LinkedList* list);

int getLengthLinkedList(LinkedList* list);
#endif
