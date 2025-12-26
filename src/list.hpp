#include <cstddef>

struct DList {
    DList *prev = NULL;
    DList *next = NULL;
};

void dlist_detach(DList *node) {
    DList *prev = node->prev;
    DList *next = node->next;

    prev->next = next;
    next->prev = prev;
}

void dlist_init(DList *node) {
    // initially, node points to itself forming a cirlce
    node->next = node->prev = node;
}

bool dlist_empty(DList *node) { return node->next == node; }

void dlist_insert_before(DList *target, DList *node) {
    DList *prev = target->prev;
    prev->next = node;
    node->next = target;
    node->prev = prev;
    target->prev = node;
}
