///// THE listbox.cpp file ////////////////////////
///   ---------------                            //
///  LIST ITEMS : used to store data allocation  //
///               in fastRam Area ONLY           //
///  YOU ARE RESPONSIBLE FOR FREEING ITEMS       //
///////////////////////////////////////////////////

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../fastram.h"
#include "cg_itemlist.h"

static char* fastStrdup(const char *s) {
    uint32_t len = 0;
    while (s[len]) len++;
    char *p = (char*)fastAlloc(len + 1);
    if (!p) return NULL;
    for (uint32_t i = 0; i <= len; i++) p[i] = s[i];
    return p;
}

static int cmp_strptr(const void *a, const void *b){
    const char *sa = *(const char* const*)a;
    const char *sb = *(const char* const*)b;

    // tiny strcmp
    while (*sa && (*sa == *sb)) { sa++; sb++; }
    return (unsigned char)*sa - (unsigned char)*sb;
}

static int listitem_ensure_cap(ItemLists_t *list, uint16_t need){
    if (need <= list->cap) return fastAllocOK;

    uint16_t newCap = (list->cap == 0) ? 8 : list->cap;
    while (newCap < need) newCap = (uint16_t)(newCap * 2);

    void *np = fastRealloc(list->items, (uint32_t)newCap * sizeof(void*));
    if (!np) return fastAllocFail;

    if (newCap > list->cap) {
        memset((void**)np + list->cap, 0, (newCap - list->cap) * sizeof(void*));
    }

    list->items = (void **)np;
    list->cap = newCap;
    return fastAllocOK;
}


int listitem_insert(ItemLists_t *list, uint16_t idx, const char *text){
    if (idx > list->count) idx = list->count;
    if (!listitem_ensure_cap(list, (uint16_t)(list->count + 1))) return fastAllocFail;

    char *dup = fastStrdup(text);
    if (!dup) return fastAllocFail;

    // shift right
    for (int i = list->count; i > (int)idx; --i)
        list->items[i] = list->items[i - 1];

    list->items[idx] = (void*)dup;
    list->count++;

    return fastAllocOK;
}

int listitem_add(ItemLists_t *list, const char *text){
    return listitem_insert(list, list->count, text);
}

int listitem_add_first(ItemLists_t *list, const char *text){
    return listitem_insert(list, 0, text);
}

void listitem_delete(ItemLists_t *list, uint16_t idx){
    if (idx >= list->count) return;

    fastFree(list->items[idx]);

    // shift left
    for (uint16_t i = idx; i + 1 < list->count; i++)
        list->items[i] = list->items[i + 1];

    list->count--;
}

void listitem_move(ItemLists_t *list, uint16_t from, uint16_t to){
    if (from >= list->count) return;
    if (to >= list->count) to = (uint16_t)(list->count - 1);
    if (from == to) return;

    void *tmp = list->items[from];

    if (from < to) {
        for (uint16_t i = from; i < to; i++)
            list->items[i] = list->items[i + 1];
        list->items[to] = tmp;
    } else {
        for (uint16_t i = from; i > to; i--)
            list->items[i] = list->items[i - 1];
        list->items[to] = tmp;
    }
}

void listitem_sort(ItemLists_t *list){
    // NOTE:
    // listitem_sort assumes items are (char*) strings.
    // This is intended ONLY for string-based lists such as ListBox or FileListBox.
    // Do NOT use on lists containing non-string pointers.

    // If you need stable sort later, we can add an index key. qsort is not stable.
    qsort(list->items, list->count, sizeof(void*), cmp_strptr);
}

void listitem_init(ItemLists_t *list){
    list->items = NULL;
    list->count = 0;
    list->cap   = 0;
}

void listitem_clear(ItemLists_t *list){
    for (uint16_t i = 0; i < list->count; i++) {
        fastFree(list->items[i]);
    }
    fastFree(list->items);

    listitem_init(list);
}

void listitem_free(ItemLists_t *list){
    if (!list) return;

    // Free all strings
    for (uint16_t i = 0; i < list->count; i++) {
        if (list->items[i]) {
            fastFree(list->items[i]);
            list->items[i] = NULL;
        }
    }

    // Free the pointer table
    if (list->items) {
        fastFree(list->items);
        list->items = NULL;
    }

    // Reset state
    list->count = 0;
    list->cap   = 0;

    fastFree(list);
}

// YOUR PROGRAM MUST FREE THIS MANUALLY WHEN YOU'RE DONE WITH IT, unless you like memory packman!
void SBOS_destroyItemList(ItemLists_t *list){
    listitem_free(list);
}

const char* listitem_get(const ItemLists_t *list, uint16_t idx){
    if (idx >= list->count) return NULL;
    return (const char *)list->items[idx];
}

uint32_t listitem_count(const ItemLists_t *list){
    return (list->count);
}

void listitem_dump(const ItemLists_t *list){
    printf("ListBox dump: %u items\n", list->count);

    for (uint16_t i = 0; i < list->count; i++) {
        const char *s = list->items[i] ? (const char *)list->items[i] : "(null)";
        printf(" [%u] %s\n", i, s);
    }
}
