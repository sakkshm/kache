#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>

struct AVLNode {
    AVLNode *parent;
    AVLNode *left;
    AVLNode *right;

    uint32_t height = 0; // subtree height
    uint32_t cnt = 0;    // subtree size
};

void avl_init(AVLNode *node) {
    node->left = node->right = node->parent = NULL;
    node->height = 1;
    node->cnt = 1;
}

uint32_t avl_height(AVLNode *node) { return node ? node->height : 0; }
uint32_t avl_cnt(AVLNode *node) { return node ? node->cnt : 0; }

void avl_update(AVLNode *node) {
    node->height =
        1 + std::max(avl_height(node->left), avl_height(node->right));
}

AVLNode *rot_left(AVLNode *node) {
    AVLNode *parent = node->parent;
    AVLNode *new_node = node->right;
    AVLNode *inner = node->right->left;

    // node <-> inner
    node->right = inner;
    if (inner) {
        inner->parent = node;
    }

    // parent <- new_node
    new_node->parent = parent;

    // new_node <-> node
    new_node->left = node;
    node->parent = new_node;

    // update height
    avl_update(node);
    avl_update(new_node);

    return new_node;
}

AVLNode *rot_right(AVLNode *node) {
    AVLNode *parent = node->parent;
    AVLNode *new_node = node->left;
    AVLNode *inner = node->left->right;

    // node <-> inner
    node->left = inner;
    if (inner) {
        inner->parent = node;
    }

    // parent <- new_node
    // may be root so might be turned to NULL by caller
    new_node->parent = parent;

    // new_node <-> node
    new_node->right = node;
    node->parent = new_node;

    // update height
    avl_update(node);
    avl_update(new_node);

    return new_node;
}

AVLNode *avl_fix_left(AVLNode *node) {
    if (avl_height(node->left->left) < avl_height(node->left->right)) {
        // LR Rotation
        node->left = rot_left(node->left);
    }

    // LL Rotation
    return rot_right(node);
}

AVLNode *avl_fix_right(AVLNode *node) {
    if (avl_height(node->left->left) > avl_height(node->left->right)) {
        // RL Rotation
        node->left = rot_right(node->right);
    }

    // RR Rotation
    return rot_left(node);
}

AVLNode *avl_fix(AVLNode *node) {
    while (true) {
        AVLNode **from = &node; // save the fixed subtree here
        AVLNode *parent = node->parent;

        if (parent) {
            // attach the fixed subtree to the parent
            from = parent->left == node ? &parent->left : &parent->right;
        } // else: save to the local variable `node`

        // auxiliary data
        avl_update(node);

        // fix the height difference of 2
        uint32_t l = avl_height(node->left);
        uint32_t r = avl_height(node->right);

        if (l == r + 2) {
            *from = avl_fix_left(node);
        } else if (l + 2 == r) {
            *from = avl_fix_right(node);
        }

        // root node, stop
        if (!parent) {
            return *from;
        }

        // continue to the parent node because its height may be changed
        node = parent;
    }
}

// detach a node where 1 of its children is empty
AVLNode *avl_del_one_child(AVLNode *node) {
    assert(!node->left || !node->right);

    AVLNode *child = node->left ? node->left : node->right;
    AVLNode *parent = node->parent;

    // update childs parent pointer
    if (child) {
        child->parent = parent; // possibly NUUL
    }

    // attach child to grandparent
    if (!parent) {
        return child; // return root node
    }

    // set pointer-to-pointer for root of subtree
    AVLNode **from = parent->left == node ? &parent->left : &parent->right;
    *from = child;

    return avl_fix(parent);
}

AVLNode *avl_del(AVLNode *node) {
    // 1 child
    if (!node->left || !node->right) {
        return avl_del_one_child(node);
    }

    // else:
    // find successor
    AVLNode *succ = node->right;
    while (succ->left) {
        succ = succ->left;
    }

    // detach the successor
    AVLNode *root = avl_del_one_child(succ);

    // swap with the successor
    *succ = *node;

    // fix child-parent relationship
    if (succ->left) {
        succ->left->parent = succ;
    }

    if (succ->right) {
        succ->right->parent = succ;
    }

    // attach succ to parent
    AVLNode **from = &root; // this line is for if its root node
    AVLNode *parent = node->parent;

    if (parent) {
        from = parent->left == node ? &parent->left : &parent->right;
    }

    *from = succ;

    return root;
}