#pragma once

#include <algorithm>

#include "avl.hpp"
#include "hashtable.hpp"
#include "utils.hpp"

struct ZSet {
    AVLNode *root = NULL; // index by (score, name), sorted
    HMap hmap;            // index by name
};

struct ZNode {
    AVLNode treeNode;
    HNode hmapNode;

    // data
    double score;
    size_t len;
    char name[0]; // flexible array, reduce memory allocation
};


bool zless(AVLNode *lhs, AVLNode *rhs);
bool z_cmp_less(AVLNode *lhs, double score, const char *name, size_t len);
void tree_insert(ZSet *zset, ZNode *znode);
void tree_dispose(AVLNode *node);
bool hcmp(HNode *node, HNode *key);

// ------------------ ZNode functions ------------------------

ZNode *znode_new(const char *name, size_t len, double score) {
    ZNode *node = (ZNode *)malloc(sizeof(ZNode) + len); // struct + name arr

    // create AVL Node
    avl_init(&node->treeNode);

    // create HMap node
    node->hmapNode.next = NULL;
    node->hmapNode.hcode = str_hash((uint8_t *)name, len);

    // add node metadata
    node->score = score;
    node->len = len;
    memcpy(&node->name, name, len);

    return node;
}

void znode_del(ZNode *node) { free(node); }

// ------------------ ZSet functions ------------------------

ZNode *zset_lookup(ZSet *zset, const char *name, size_t len) {
    if (!zset->root) {
        return NULL;
    }

    HKey key;
    key.node.hcode = str_hash((uint8_t *)name, len);
    key.name = name;
    key.len = len;

    HNode *found = hm_lookup(&zset->hmap, &key.node, &hcmp);
    return found ? container_of(found, ZNode, hmapNode) : NULL;
}

void zset_update(ZSet *zset, ZNode *node, double score) {
    if (node->score == score) {
        // no update needed
        return;
    }

    // detach node
    zset->root = avl_del(&node->treeNode);

    // reinsert node
    avl_init(&node->treeNode);
    node->score = score;
    tree_insert(zset, node);
}

bool zset_insert(ZSet *zset, const char *name, size_t size, double score) {
    if (ZNode *node = zset_lookup(zset, name, size)) {
        zset_update(zset, node, score);
        return false;
    }

    ZNode *node = znode_new(name, size, score);
    hm_insert(&zset->hmap, &node->hmapNode);
    tree_insert(zset, node);

    return true;
}

void zset_delete(ZSet *zset, ZNode *node) {
    // remove from hashtable
    HKey key;
    key.len = node->len;
    key.name = node->name;
    key.node.hcode = node->hmapNode.hcode;

    HNode *found = hm_delete(&zset->hmap, &key.node, &hcmp);
    assert(found);

    // remove from tree
    zset->root = avl_del(&node->treeNode);

    // deallocate node
    znode_del(node);
}

// Seek to the first pair where pair >= (score, name)
ZNode *zset_seek(ZSet *zset, double score, const char *name, size_t len) {
    AVLNode *found = NULL;
    for (AVLNode *node = zset->root; node;) {
        if (z_cmp_less(node, score, name, len)) {
            node = node->right;
        } else {
            found = node; // candidate
            node = node->left;
        }
    }
    return found ? container_of(found, ZNode, treeNode) : NULL;
}

// Iterate to the n-th successor/predecessor (offset)
ZNode *znode_offset(ZNode *node, int64_t offset) {
    AVLNode *tnode = node ? avl_offset(&node->treeNode, offset) : NULL;
    return tnode ? container_of(tnode, ZNode, treeNode) : NULL;
}

// ---------------- Helper Functions ------------------

// Helper for hashtable key compare
bool hcmp(HNode *node, HNode *key) {
    ZNode *znode = container_of(node, ZNode, hmapNode);
    HKey *hkey = container_of(key, HKey, node);

    if (znode->len != hkey->len) {
        return false;
    }

    return memcmp(znode->name, hkey->name, znode->len) == 0;
}

// zset tuple comparison
bool zless(AVLNode *lhs, AVLNode *rhs) {
    ZNode *zl = container_of(lhs, ZNode, treeNode);
    ZNode *zr = container_of(rhs, ZNode, treeNode);

    if (zl->score != zr->score) {
        return zl->score < zr->score;
    }

    int rv = memcmp(zl->name, zr->name, std::min(zl->len, zr->len));
    return (rv != 0) ? (rv < 0) : (zl->len < zr->len);
}

// zset component compare
bool z_cmp_less(AVLNode *lhs, double score, const char *name, size_t len) {
    ZNode *zl = container_of(lhs, ZNode, treeNode);

    if (zl->score != score) {
        return zl->score < score;
    }

    int rv = memcmp(zl->name, name, std::min(zl->len, len));
    return (rv != 0) ? (rv < 0) : (zl->len < len);
}

void tree_insert(ZSet *zset, ZNode *znode) {
    AVLNode *parent = NULL;
    AVLNode **from = &zset->root;

    while (*from) {
        parent = *from;
        from = zless(&znode->treeNode, parent) ? &parent->left : &parent->right;
    }

    *from = &znode->treeNode;
    znode->treeNode.parent = parent;
    zset->root = avl_fix(&znode->treeNode);
}

void tree_dispose(AVLNode *node) {
    if (!node) {
        return;
    }
    tree_dispose(node->left);
    tree_dispose(node->right);
    znode_del(container_of(node, ZNode, treeNode));
}

// destroy the zset
void zset_clear(ZSet *zset) {
    hm_clear(&zset->hmap);
    tree_dispose(zset->root);
    zset->root = NULL;
}