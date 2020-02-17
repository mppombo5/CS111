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

    SortedList_delete(&elmt4);
    SortedList_delete(&elmt7);
    // should no longer have "kenobi!" or "c"
    cur = list;
    do {
        cur = cur->next;
        printf("%s\n", cur->key);
    } while (cur->next != list);

    SortedListElement_t* look1 = SortedList_lookup(list, "d");
    printf("%s\n", look1->prev->key);
    printf("%s\n", look1->key);
    printf("%s\n", look1->next->key);

    return 0;
}
