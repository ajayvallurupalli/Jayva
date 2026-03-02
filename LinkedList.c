#include <stdlib.h>
#include "LinkedList.h"

Cons _mkCons(void* val) {
        Cons result;
        result.val = val;
        result.next = NULL;
        return result;
}

LinkedList mkLinkedList() {
        LinkedList result;
        result.list = NULL;
        return result;
}

void _freeCons(Cons* cons) {
	if (cons) { /*if not  empty list  */
		if (cons->next) _freeCons(cons->next);
		free(cons);
	}
}

void freeLinkedListInners(LinkedList* list) {
	_freeCons(list->list);
}

LinkedList consLinkedList(LinkedList list, void* val) {
        Cons* next = malloc(sizeof(Cons));
        *next = _mkCons(val);

        next->next = list.list;
        list.list = next;
        return list;
}

void consLinkedListByRef(LinkedList* list, void* val) {
        Cons* next = malloc(sizeof(Cons));
        *next = _mkCons(val);

        next->next = list->list;
        list->list = next;
}

void* _getElementOfCons(Cons* cons, void* context, int (*pred) (void*, void*)) {
	if           (!cons)                          return NULL;
	else if      (pred(context, cons->val))       return cons->val;
	else                                          return _getElementOfCons(cons->next, context, pred);
}

/* Returns NULL pointer if no elements follow pred
 * Predicate goes (context, value), be careful with void*s */
void* getElementOfLinkedList(LinkedList* list, void* context, int (*pred) (void*, void*)) {
	return _getElementOfCons(list->list, context, pred);

}

void _mapCons(Cons* cons, void (*map) (void*)) {
	if (!cons) return;
	else {
		map(cons->val);
		_mapCons(cons->next, map);
	}
}

void mapLinkedList(LinkedList* list, void (*map) (void*)) {
	_mapCons(list->list, map);
}

Cons* reverseLinkedListGo(Cons* main, Cons* prev) {
	Cons* result;
	if (main->next) result = reverseLinkedListGo(main->next, main);
	else result = main;

	main->next = prev;
	return result;
}

void reverseLinkedList(LinkedList* list) {
	if (!list->list || !list->list->next) return; /* if 0 or 1 length*/
	list->list = reverseLinkedListGo(list->list, NULL);
}

int getLengthLinkedList(LinkedList* list) {
	int length = 0;
	Cons* recurse = list->list;

	while (recurse) {
		length++;
		recurse = recurse->next;
	}

	return length;
}
