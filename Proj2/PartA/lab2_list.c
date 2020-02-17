/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include "SortedList.h"

int main() {
    SortedList_t list_t;
    SortedList_t* list = &list_t;
    list->next = list;
    list->prev = list;
    list->key = NULL;

    SortedListElement_t elmt1;
    elmt1.key = "hello";
    SortedListElement_t elmt2;
    elmt2.key = "there!";
    SortedListElement_t elmt3;
    elmt3.key = "general";
    SortedListElement_t elmt4;
    elmt4.key = "kenobi!";
    SortedListElement_t elmt5;
    elmt5.key = "a";
    SortedListElement_t elmt6;
    elmt6.key = "b";
    SortedListElement_t elmt7;
    elmt7.key = "c";
    SortedListElement_t elmt8;
    elmt8.key = "d";
    SortedListElement_t elmt9;
    elmt9.key = "z";

    SortedList_insert(list, &elmt1);
    SortedList_insert(list, &elmt2);
    SortedList_insert(list, &elmt3);
    SortedList_insert(list, &elmt4);
    SortedList_insert(list, &elmt5);
    SortedList_insert(list, &elmt6);
    SortedList_insert(list, &elmt7);
    SortedList_insert(list, &elmt8);
    SortedList_insert(list, &elmt9);

    SortedListElement_t* cur = list;
    do {
        cur = cur->next;
        printf("%s\n", cur->key);
    } while (cur->next != list);
    printf("\n");
    // now backwards
    cur = list;
    do {
        cur = cur->prev;
        printf("%s\n", cur->key);
    } while (cur->prev != list);
    printf("\n");
    printf("size %d\n\n", SortedList_length(list));

    SortedList_delete(&elmt4);
    SortedList_delete(&elmt7);
    // should no longer have "kenobi!" or "c"
    cur = list;
    do {
        cur = cur->next;
        printf("%s\n", cur->key);
    } while (cur->next != list);
    printf("\n");
    cur = list;
    do {
        cur = cur->prev;
        printf("%s\n", cur->key);
    } while (cur->prev != list);
    printf("\n");
    printf("size %d\n\n", SortedList_length(list));

    SortedListElement_t* look1 = SortedList_lookup(list, "d");
    printf("%s\n", look1->prev->key);
    printf("%s\n", look1->key);
    printf("%s\n", look1->next->key);

    SortedListElement_t* look2 = SortedList_lookup(list, "hello");
    printf("%s\n", look2->prev->key);
    printf("%s\n", look2->key);
    printf("%s\n", look2->next->key);

    SortedList_t list2_t;
    SortedList_t* list2 = &list2_t;
    list2->next = list2;
    list2->prev = list2;
    printf("\nsize list2 %d\n", SortedList_length(list2));

    return 0;
}
