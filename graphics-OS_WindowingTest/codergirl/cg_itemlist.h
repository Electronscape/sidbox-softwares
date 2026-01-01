#ifndef CG_ITEMLIST_H
#define CG_ITEMLIST_H

#include <stdint.h>

// - object freeing function Pointer! MUST use these if we have gadgets attached here, but for now not used
//typedef void (*ItemFreeFn)(void *p);
//typedef int  (*ItemCmpFn)(const void *a, const void *b);

typedef struct {
    void     **items;     // array of pointers to FASTRAM strings
    uint16_t count;
    uint16_t cap;


    // object free too -- implement IF we need them
    //ItemFreeFn free_fn;
    //ItemCmpFn  cmp_fn;
} ItemLists_t;




// INTERNALS ------------------------------------------------------------------------------------------------------
void listitem_init(ItemLists_t *list);
void listitem_clear(ItemLists_t *list);

void listitem_free(ItemLists_t *list);


void listitem_dump(const ItemLists_t *list);

void listitem_move(ItemLists_t *list, uint16_t from, uint16_t to);
void listitem_delete(ItemLists_t *list, uint16_t idx);
int  listitem_insert(ItemLists_t *list, uint16_t idx, const char *text);
int  listitem_add(ItemLists_t *list, const char *text);
int  listitem_add_first(ItemLists_t *list, const char *text);

void listitem_sort(ItemLists_t *list);
const char* listitem_get(const ItemLists_t *list, uint16_t idx);
uint32_t listitem_count(const ItemLists_t *list);


// API INTERFACES -------------------------------------------------------------------------------------------------
void SBOS_destroyItemList(ItemLists_t *list);
const char* SBOS_listitem_getString(const ItemLists_t *list, int index);
void listitem_deinit(ItemLists_t *list);    // hmm feels like this should be internal


#endif // CG_ITEMLIST_H
