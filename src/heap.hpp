#include <cstddef>
#include <cstdint>
#include <vector>

/*
    Min Heap:
    - A node’s value is less than both of its children.
    - Each level is fully filled except for the last level.
*/

struct HeapItem {
    uint64_t val;       // heap value
    size_t *ref = NULL; // points to Entry::heap_idx
};

size_t heap_left(size_t i) { return 2 * i + 1; }

size_t heap_right(size_t i) { return 2 * i + 2; }

size_t heap_parent(size_t i) { return ((i + 1) / 2) - 1; }

void heap_up(HeapItem *a, size_t pos) {
    HeapItem t = a[pos];

    while (pos > 0 && a[heap_parent(pos)].val > t.val) {
        // swap with parent
        a[pos] = a[heap_parent(pos)];
        *a[pos].ref = pos;

        pos = heap_parent(pos);
    }

    a[pos] = t;
    *a[pos].ref = pos;
}

void heap_down(HeapItem *a, size_t pos, size_t len) {
    HeapItem t = a[pos];
    while (true) {
        // find the smallest one among the parent and their kids
        size_t l = heap_left(pos);
        size_t r = heap_right(pos);
        size_t min_pos = pos;
        uint64_t min_val = t.val;

        if (l < len && a[l].val < min_val) {
            min_pos = l;
            min_val = a[l].val;
        }
        if (r < len && a[r].val < min_val) {
            min_pos = r;
        }
        if (min_pos == pos) {
            break;
        }
        // swap with the kid
        a[pos] = a[min_pos];
        *a[pos].ref = pos;
        pos = min_pos;
    }

    a[pos] = t;
    *a[pos].ref = pos;
}

void heap_update(HeapItem *a, size_t pos, size_t len) {
    if (pos > 0 && a[heap_parent(pos)].val > a[pos].val) {
        heap_up(a, pos);
    } else {
        heap_down(a, pos, len);
    }
}

void heap_delete(std::vector<HeapItem> &a, size_t pos) {
    // swap the erased item with the last item
    a[pos] = a.back();
    a.pop_back();

    // update the swapped item
    if (pos < a.size()) {
        heap_update(a.data(), pos, a.size());
    }
}

// update or append
void heap_upsert(std::vector<HeapItem> &a, size_t pos, HeapItem t) {
    if (pos < a.size()) {
        a[pos] = t; // update an existing item
    } else {
        pos = a.size();
        a.push_back(t); // or add a new item
    }
    heap_update(a.data(), pos, a.size());
}