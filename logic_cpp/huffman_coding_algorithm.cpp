// Core Logic behind Huffman Coding Tree
// The methods are called internally by the `HuffmanSimulator` class inside `wasm_bindings.cpp` when React triggers a step in the simulation

#include "huffman_coding.hpp"
#include <utility> // for std::swap

HeapNode::HeapNode(char a, int f) : alphabet(a), freq(f), left(nullptr), right(nullptr) {} // Constructor for a node

MinHeap::MinHeap(int s) : size(s) // Constructor to allocate a block of memory inside the WASM ArrayBuffer
{
    arr = new HeapNode *[size];
}

MinHeap::~MinHeap() // Destructor to clean up that block of memory back to the ArrayBuffer when the simulator is destroyed -> This does not delete the actual nodes, just the array holding the pointers to them
{
    delete[] arr;
}

void MinHeap::minHeapify(int idx) // an internal helper
{
    int smallest = idx;
    int left = L(idx);
    int right = R(idx);

    if (left < size && arr[left]->freq < arr[smallest]->freq)
        smallest = left;
    if (right < size && arr[right]->freq < arr[smallest]->freq)
        smallest = right;

    if (smallest != idx)
    {
        std::swap(arr[idx], arr[smallest]);
        minHeapify(smallest);
    }
}

void MinHeap::buildHeap()
{
    for (int i = P(size - 1); i >= 0; --i) // calling minHeapify for each node from the last non-leaf node (parent of the last node) to the root
    {
        minHeapify(i);
    }
}

HeapNode *MinHeap::extractMin() // returns a pointer to the minNode
{
    if (size <= 0)
        return nullptr;

    // Moving the last leaf to the root -> Shrinking the heap -> Bubbling down the new root
    HeapNode *temp = arr[0];
    arr[0] = arr[size - 1];
    --size;
    minHeapify(0);

    return temp;
}

void MinHeap::insertNode(HeapNode *node) // inserting a node into the heap following the minHeap property
{
    ++size;
    int i = size - 1;

    // Bubbling up the newly inserted node to its correct position
    while (i > 0 && node->freq < arr[P(i)]->freq)
    {
        arr[i] = arr[P(i)];
        i = P(i);
    }
    arr[i] = node;
}

int MinHeap::getSize() const // returns heap size
{
    return size;
}

void MinHeap::setNode(int index, HeapNode *node) // placing a node
{
    arr[index] = node;
}

HeapNode *MinHeap::getNode(int index) const // rendering a node
{
    return arr[index];
}