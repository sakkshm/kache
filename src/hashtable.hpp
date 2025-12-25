#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>

// container_of macro to access data of intrusive data dtructure
#define container_of(ptr, T, member) \
    ((T *)( (char *)ptr - offsetof(T, member) ))

struct HNode {
    HNode *next = NULL;
    uint64_t hcode = 0; // hash value
};

struct HTab {
    HNode **tab = NULL; // arr of buckets
    size_t mask = 0;    // power of 2 for arr size (n)
    size_t size = 0;    // number of keys
};

// Resizable HashMap
// Contains 2 HTab for progressive rehashing
struct HMap {
    HTab newer;
    HTab older;
    size_t migrate_pos = 0;
};

// Helper for hashtable key compare
struct HKey {
    HNode node;
    const char *name = NULL;
    size_t len = 0;
};

const size_t k_max_load_factor = 8;
const size_t k_rehashing_work = 128; // constant work

// HTab methods

void h_init(HTab *htab, size_t n) {
    // n should be power of 2
    assert(n > 0 && ((n - 1) & n) == 0);

    // use calloc so all default values are null
    htab->tab = (HNode **)calloc(n, sizeof(HNode *));
    htab->mask = n - 1;
    htab->size = 0;
}

void h_insert(HTab *htab, HNode *node) {
    // hash function: instead of mod we use bit operations
    // hash(key): key % size <=> hcode & (n-1)
    // since hcode is size is a power of 2

    // find which bucket to go in
    size_t pos = node->hcode & htab->mask;

    // add new node to front, O(1)
    HNode *curr = htab->tab[pos];
    node->next = curr;
    htab->tab[pos] = node;
    htab->size++;
}

// hashtable look up subroutine.
// It returns the address of
// the parent pointer that owns the target node,
// which can be used to delete the target node.
HNode **h_lookup(HTab *htab, HNode *key, bool (*eq)(HNode *, HNode *)) {
    if (!htab->tab) {
        return NULL;
    }

    size_t pos = key->hcode & htab->mask;

    HNode **from = &htab->tab[pos];
    for (HNode *curr; (curr = *from) != NULL; from = &curr->next) {
        if (curr->hcode == key->hcode && eq(curr, key)) {
            return from; // return the addr of parent pointer
        }
    }

    return NULL;
}

HNode *h_detach(HTab *htab, HNode **from) {
    HNode *node = *from; // the target node
    *from = node->next;  // update the incoming pointer to the target
    htab->size--;
    return node;
}

bool h_foreach(HTab *htab, bool (*f)(HNode *, void *), void* arg){
    for(size_t i = 0; i <= htab -> mask; i++){
        for(HNode *node = htab->tab[i]; node != NULL; node = node -> next){
            if(!f(node, arg)){
                return false;
            }
        }
    }

    return true;
}

// HMap methods

// Normally, newer is used and older is unused
// When load factor is too high, we rehash
// newer is moved to older, newer is replaced by larger table
void hm_trigger_rehashing(HMap *hmap) {
    hmap->older = hmap->newer; // (newer, older) <- (new_table, newer)
    h_init(&hmap->newer, (hmap->newer.mask + 1) * 2);
    hmap->migrate_pos = 0;
}

void hm_help_rehashing(HMap *hmap) {
    size_t nwork = 0;

    while (nwork < k_rehashing_work && hmap->older.size > 0) {
        // find a non empty slot
        HNode **from = &hmap->older.tab[hmap->migrate_pos];

        if (!*from) {
            hmap->migrate_pos++;
            continue; // empty slot
        }

        // move the first list item to the newer table
        h_insert(&hmap->newer, h_detach(&hmap->older, from));
        nwork++;
    }

    // discard old table if all data is rehashed
    if(hmap->older.size == 0 && hmap->older.tab){
        free(hmap->older.tab);
        hmap->older = HTab{};
    }
}

// during rehashing, we might need to lookup both
// newer/older tables during lookup/delete
HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)) {
    hm_help_rehashing(hmap);

    HNode **from = h_lookup(&hmap->newer, key, eq);
    if (!from) {
        from = h_lookup(&hmap->older, key, eq);
    }
    return from ? *from : NULL;
}

HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)) {
    hm_help_rehashing(hmap);

    if (HNode **from = h_lookup(&hmap->newer, key, eq)) {
        return h_detach(&hmap->newer, from);
    }
    if (HNode **from = h_lookup(&hmap->older, key, eq)) {
        return h_detach(&hmap->older, from);
    }
    return NULL;
}

void hm_insert(HMap *hmap, HNode *node) {
    if (!hmap->newer.tab) {
        h_init(&hmap->newer, 4); // init if empty
    }

    h_insert(&hmap->newer, node); // always insert in newer table

    // check if we need to rehash
    // done when older table is deleted after transferring all keys
    if (!hmap->older.tab) {
        size_t threshold = (hmap->newer.mask + 1) * k_max_load_factor;
        if (hmap->newer.size >= threshold) {
            hm_trigger_rehashing(hmap);
        }
    }

    hm_help_rehashing(hmap);
}

void hm_clear(HMap *hmap) {
    free(hmap->newer.tab);
    free(hmap->older.tab);
    *hmap = HMap{};
}

void hm_foreach(HMap *hmap, bool (*f)(HNode *, void *), void *arg){
    h_foreach(&hmap->newer, f, arg);
    h_foreach(&hmap->older, f, arg);
}

size_t hm_size(HMap *hmap) {
    return hmap->newer.size + hmap->older.size;
}