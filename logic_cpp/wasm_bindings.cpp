// This file acts as the bridge file, as control and translator between the C++ MinHeap logic and the React frontend. It is the absolute core of your WebAssembly bridge, which makes it possible for the web browser to actually see and control the backend logic.

// JavaScript (React) calls the methods exposed in EMSCRIPTEN_BINDINGS -> When React calls `simulator.stepForward()`, it hits the C++ `HuffmanSimulator::stepForward()`, which in turn manipulates the native C++ `MinHeap` and `HeapNode` objects

#include "huffman_coding.hpp"
#include <emscripten/bind.h> // Emscripten's embind library for binding C++ to JavaScript
#include <vector>
#include <string>

using namespace emscripten;

struct UINode // a node in the frontend -> explicitly strips away raw memory pointers (*left, *right) of C++ (as passing raw C++ pointers to JavaScript is dangerous and can lead to memory access violations or complex garbage collection issues in the browser)
{
    std::string character;
    int freq;
};

class HuffmanSimulator
{
private:
    MinHeap *heap; // Pointer to the native C++ MinHeap instance allocated in the WebAssembly sandbox
    bool isDone;   // Flag for tracking the completion step so React knows when the tree is finished

    std::string nodeToJSON(HeapNode *node) const // JSON Serialization approach - Recursive function to convert a C++ tree into a JSON-like string -> called internally by getTreesJSON()
    {
        if (!node) // Base case : if ptr becomes null, the JSON equivalent is returned
            return "null";

        // Concatenation (The backslash (\) is an "escape character" -> it tells C++ to add double quote literally instead of ending the string)
        std::string json = "{";
        json += "\"char\": \"" + std::string(1, node->alphabet) + "\", ";
        json += "\"freq\": " + std::to_string(node->freq);

        if (node->left || node->right) // If the node has any children -> they are recursively collected
        {
            json += ", \"children\": [";
            json += nodeToJSON(node->left) + ", ";
            json += nodeToJSON(node->right);
            json += "]";
        }
        json += "}";
        // Output in JSON-like format looks like, {"char": "A", "freq": 5}
        return json;
    }

public:
    HuffmanSimulator(std::vector<std::string> chars, std::vector<int> freqs) // Constructor : called by React via `new wasmModule.HuffmanSimulator(chars, freqs)`
    {
        int initialSize = chars.size();
        heap = new MinHeap(initialSize); // Dynamic allocation of MinHeap in WebAssembly memory buffer
        isDone = false;

        for (int i = 0; i < initialSize; ++i)
        {
            heap->setNode(i, new HeapNode(chars[i][0], freqs[i])); // JavaScript strings are passed as std::string -> chars[i][0] is used to extract just the first C++ 'char' to store in the HeapNode
        }
        heap->buildHeap(); // minHeap -> O(n)
    }

    ~HuffmanSimulator() { delete heap; } // Destructor : called by React via `simulator.delete()` to free WASM memory -> prevents memory leaks in the WASM ArrayBuffer

    // stepForward() function is called iteratively by React to generate the whole Huffman Tree -> finally only one element remains in the array -> it is the root of the tree -> the rest of the tree branches from arr[0]

    bool stepForward() // moving to the next state of the Huffman Tree -> every time React calls this, it executes exactly one step of the Huffman process
    {
        if (heap->getSize() <= 1) // check if the tree has been completely formed , i.e., one ele is left ,i.e., the root
        {
            isDone = true;
            return false;
        }

        // Children are extracted -> Frequencies are added -> a helper parent node(labeled '$') is generated and then put into the heap (in the position it belongs according to minHeap properties) -> the extracted nodes are added as the left and the right child of the helper node

        HeapNode *left = heap->extractMin();
        HeapNode *right = heap->extractMin();

        HeapNode *top = new HeapNode('$', left->freq + right->freq); // Helper Parent Node
        top->left = left;
        top->right = right;

        heap->insertNode(top); // O(logn)
        return true;
    }

    std::vector<UINode> getHeapState() const // loops through the current MinHeap array and packages each element into UINode struct -> returns a vector by value, which Emscripten automatically translates into a JS array for rendering
    {
        std::vector<UINode> uiState;
        for (int i = 0; i < heap->getSize(); ++i)
        {
            HeapNode *node = heap->getNode(i);
            UINode uiNode;
            uiNode.character = std::string(1, node->alphabet); // Convert char to string for JS
            uiNode.freq = node->freq;
            uiState.push_back(uiNode);
        }
        return uiState;
    }

    std::string getTreesJSON() const // Function exposed to React that returns the whole Forest as JSON -> as passing the deeply nested Huffman Tree via std::vector is highly complex, getTreesJSON() packages the entire forest into a JSON array string and sends that single text string to React, where it can be easily parsed with JSON.parse()
    {
        std::string json = "[";
        for (int i = 0; i < heap->getSize(); ++i)
        {
            json += nodeToJSON(heap->getNode(i));
            if (i < heap->getSize() - 1) // Appending a comma only if it is NOT the last element, preventing trailing comma JSON syntax errors
                json += ", ";
        }
        json += "]";
        return json;
    }

    bool getIsDone() const { return isDone; } // Getter to allow React to check the completion status
};

// WebAssembly strips away all C++ function names when it compiles to binary. This block acts as the Emscripten Binding Dictionary, telling the Emscripten compiler exactly how to wire everything back up for JavaScript. This maps C++ types and classes to JavaScript accessible objects.
EMSCRIPTEN_BINDINGS(huffman_module)
{
    // If C++ returns a UINode, it is converted to a JavaScript object with character and freq properties
    value_object<UINode>("UINode")
        .field("character", &UINode::character)
        .field("freq", &UINode::freq);

    // Instructs Emscripten to create special JavaScript classes (VectorString, VectorInt, VectorUINode) that allow React to dynamically allocate and manipulate C++ arrays directly within the WebAssembly memory heap
    register_vector<UINode>("VectorUINode");
    register_vector<std::string>("VectorString");
    register_vector<int>("VectorInt");

    // Maps C++ class methods to JavaScript methods, allowing React to physically call .stepForward() or .getTreesJSON() on the instantiated JS object
    class_<HuffmanSimulator>("HuffmanSimulator")
        .constructor<std::vector<std::string>, std::vector<int>>()
        .function("stepForward", &HuffmanSimulator::stepForward)
        .function("getHeapState", &HuffmanSimulator::getHeapState)
        .function("getTreesJSON", &HuffmanSimulator::getTreesJSON)
        .function("getIsDone", &HuffmanSimulator::getIsDone);
}