// Header file ->  It is included by `huffman_coding_algorithm.cpp` and `wasm_bindings.cpp`

#pragma once // C++ preprocessor directive -> prevents double inclusion that avoids duplicate definition errors

// Inline functions for defining the relation between nodes in an array(0-indexed) for heap
inline int P(int i) { return (i - 1) / 2; } // Returns the index of the Parent node
inline int L(int i) { return (2 * i) + 1; } // Returns the index of the Left child
inline int R(int i) { return (2 * i) + 2; } // Returns the index of the Right child

class HeapNode // Each node of the tree
{
public:
    char alphabet;
    int freq;
    HeapNode *left, *right; // pointer to the left and right children -> to construct the actual final Huffman Tree structure

    HeapNode(char a, int f); // Constructor
};

class MinHeap // The heap to be required for Heapify
{
private:
    int size;       // count of elements in the heap
    HeapNode **arr; // An array of pointer to heapNodes -> 'HeapNode*' is a pointer to a node, 'HeapNode**' is a dynamic array that holds pointers to nodes

    void minHeapify(int idx); // an internal helper for min Heapificattion (bubbling down)

public:
    MinHeap(int s); // Constructor to allocate memory
    ~MinHeap();     // Destructor to clean up the array

    void buildHeap();                // Converts a random, unsorted array into a MinHeap -> O(n)
    HeapNode *extractMin();          // pops and returns a pointer to the minNode -> O(lgn)
    void insertNode(HeapNode *node); // pushes a new node into the heap (bubbling up) -> O(lgn)

    // Accessors (Getters & Setters) used primarily by the WebAssembly bindings to read the state of C++ heap and send it to JS
    int getSize() const;                     // size of heap
    void setNode(int index, HeapNode *node); // setter for the nodes
    HeapNode *getNode(int index) const;      // getter for the bindings
};