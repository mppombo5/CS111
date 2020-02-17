/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include <string.h>
#include "SortedList.h"

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    SortedListElement_t* cur = list;
    const char* key = element->key;
    do {
        cur = cur->next;
    } while (cur != list && (strcmp(cur->key, key) < 0));

    element->next = cur;
    element->prev = cur->prev;
    cur->prev->next = element;
    cur->prev = element;
}

int SortedList_delete(SortedListElement_t *element) {
    SortedListElement_t* prev = element->prev;
    SortedListElement_t* next = element->next;
    if (prev->next != element || next->prev != element) {
        return 1;
    }
    prev->next = next;
    next->prev = prev;
    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    SortedListElement_t* cur = list->next;
    while (cur != list) {
        if (strcmp(key, cur->key) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

int SortedList_length(SortedList_t *list) {
    SortedListElement_t* cur = list;
    if (cur->prev->next != cur || cur->next->prev != cur) {
        return -1;
    }
    int size = 0;
    cur = cur->next;
    while (cur != list) {
        size++;
        cur = cur->next;
    }
    return size;
}
