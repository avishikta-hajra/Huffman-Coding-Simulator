# Huffman Coding Tree Simulator

An interactive, high-performance web-based simulator for visualizing the Huffman Coding algorithm. This project leverages a **C++ backend** compiled to **WebAssembly (WASM)** for raw computational speed and memory management, paired with a **React** frontend for smooth UI animations.

---

## 1. The Huffman Coding Problem

Huffman Coding is a highly efficient, lossless data compression algorithm. It reduces the size of data by assigning variable-length binary codes to input characters based on their frequencies of occurrence. 

### Working Principle of the Algorithm :
1. **Frequency Analysis :** Characters that appear frequently (like 'e' or 't' in English) are assigned very short binary codes (e.g., `01`). Characters that appear rarely are assigned longer codes (e.g., `11010`).
2. **Min-Heap Construction :** Every character is turned into a leaf node and placed into a Priority Queue (Min-Heap) ordered by frequency.
3. **Tree Building (Extract & Merge) :** The algorithm repeatedly extracts the two nodes with the lowest frequencies, merges them into a new internal node (whose frequency is the sum of the two), and pushes it back into the heap.
4. **Final Code Generation :** Once only one node remains (the root), the tree is complete. Traversing down the left branch appends a `0`, and the right branch appends a `1`, forming the final binary codes for each leaf character.

---

## 2. Simulation Architecture

This project leverages a hybrid architecture : **C++ handles the algorithmic logic**, while **React handles the state tracking and visual rendering**.

### File Structure & Functions :
* **`logic_cpp/huffman_coding.hpp` & `huffman_coding_algorithm.cpp` :** The core C++ implementation. Defines the `HeapNode` and the `MinHeap` data structures. Contains the low-level logic for `minHeapify`, `extractMin`, and `insertNode`.
* **`logic_cpp/wasm_bindings.cpp` :** The absolute core of the WebAssembly bridge. Uses Emscripten's Embind to expose the C++ `HuffmanSimulator` class to JavaScript. Crucially, it contains `getTreesJSON()`, which serializes the complex C++ memory pointers into a flat JSON string that React can safely parse.
* **`logic_cpp/build.sh` :** The shell script that executes the Emscripten compiler (`emcc`) to generate the WebAssembly binary.
* **`src/App.jsx` :** The React frontend. Manages the UI state, user inputs, history caching, and recursive rendering of the `<HuffmanTree />` component.
* **`src/index.css` :** Custom styling using CSS pseudo-elements to draw the  arrays and tree branches (connectors and stems).

### Flow of Execution :

1. **Initialization :** When the React app mounts, it asynchronously loads the auto-generated `huffman_coding.js` glue code, attaching the WebAssembly module to `window.HuffmanCodingModule`.
2. **Min Heapify (Bootstrapping) :**
   When the user clicks "Min Heapify", React parses the input arrays, allocates C++ vectors in the WASM memory buffer, and calls `new wasmModule.HuffmanSimulator()`. The C++ backend immediately builds the initial Min-Heap. React then calls `sim.getTreesJSON()`, parses the JSON into `forestState`, and deletes the initialization vectors to prevent memory leaks.
3. **Extract & Merge (Stepping Forward) :**
   Clicking the `>` (Next) button triggers `simulator.stepForward()` in C++. The C++ backend extracts the two minimum nodes, merges them into a `$` node, and re-inserts it. React fetches the updated JSON, updates the UI, and pushes this new state into a `history` array.
4. **Previous State (Stepping Backward):**
   Because the C++ algorithm only runs *forward*, clicking the `<` (Previous) button does not interact with WebAssembly. Instead, React decrements a `currentStep` index and loads the previous tree state entirely from the cached `history` array.

### A Detailed Example Walkthrough
Given the input:
* **Characters:** `a, b, c, d, e, f, g`
* **Frequencies:** `13, 2, 17, 5, 11, 3, 7`

1. **Start (Min Heapify) :** The user clicks "Min Heapify". The C++ backend takes the unsorted arrays and builds a priority queue (Min-Heap). The UI displays a flat array of 7 disconnected leaf nodes, now structurally ordered so the smallest frequencies are ready to be extracted.
2. **Step 1 :** The backend extracts the two absolute minimum nodes from the heap: `b (2)` and `f (3)`. It merges them into an internal node `$ (5)`. The array now has 6 elements. React smoothly animates `b` and `f` sliding underneath the new `$`.
3. **Step 2 :** The backend extracts the next two minimums: `d (5)` and the newly created `$ (5)`. It merges them into `$ (10)`.
4. **Step 3 :** The backend extracts `g (7)` and `$ (10)`, merging them into `$ (17)`.
5. **Step 4 :** The algorithm extracts `e (11)` and `a (13)` (which are now the smallest in the heap), merging them into a new separate internal node `$ (24)`.
6. **Step 5 :** The backend extracts `$ (17)` and `c (17)`, merging them into `$ (34)`.
7. **Step 6 :** Finally, it extracts the last two remaining nodes, `$ (24)` and `$ (34)`, merging them into the final root node `$ (58)`. The tree is now complete.
8. **Adding Labels :** The user clicks "Add Label". React recursively passes a `path` string prop down the completed tree (`"0"` for the left branch, `"1"` for the right branch). The original leaf nodes (like `b` or `c`) receive their final binary string (e.g., `000` or `11`) and display it.

---

### WebAssembly Initialization & Boot Process

Here is how the boot sequence works under the hood:
* **Component Mount :** When the React application loads, the main `App` component mounts with a null WebAssembly module state, in a loading screen.
* **Factory Invocation :** A `useEffect` hook immediately fires, invoking the Emscripten-generated factory function `window.HuffmanCodingModule()`.
* **Binary Compilation :** The browser asynchronously fetches the `huffman_coding.wasm` binary file over the network and compiles it into executable machine code using `WebAssembly.instantiateStreaming`.
* **Environment Setup :** The Emscripten runtime allocates the WebAssembly memory heap and links necessary C++ standard library functions (like `malloc` and `free`).
* **State Hydration :** Once compilation is complete, the Promise resolves, passing the fully instantiated module back to React. React updates its state (`setWasmModule`), unmounts the loading screen, and renders the interactive control panel.