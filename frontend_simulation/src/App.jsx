import { useEffect, useState } from 'react';
import { motion } from 'framer-motion';
import './index.css';

// WASM Workings : the Frontend UI initializes the Emscripten module `window.HuffmanCodingModule()` -> when user clicks 'Min Heapify': It creates C++ Vectors, passes them to `new wasmModule.HuffmanSimulator()`, and then DELETES the vectors from WASM memory -> User clicks 'Extract & Merge': It calls `simulator.stepForward()` (C++) and then `simulator.getTreesJSON()` (C++) to update the React state

const HuffmanTree = ({ node, isRoot, path = "", showLabels }) => {
  if (!node) return null;
  const isInternal = node.char === '$';

  return (
    <div className="tree-node-wrapper">
      <div className={`node-base ${isRoot ? 'node-rect' : 'node-circle'} ${isInternal ? 'internal-color' : 'leaf-color'}`}>
        <span className="node-char">{isInternal ? '' : `'${node.char}'`}</span>
        <span className="node-freq">{node.freq}</span>

        {!isInternal && showLabels && (
          <span className="final-code-label">{path}</span>
        )}
      </div>

      {node.children && (
        <div className={`children-container ${isRoot ? 'is-root-children' : ''}`}>
          <div className="child-wrapper">
            <span className="branch-label left-label">0</span>
            <svg className="drop-arrow" width="10" height="24">
              <line x1="5" y1="0" x2="5" y2="21" stroke="#64748b" strokeWidth="2" markerEnd="url(#arrowhead)" />
            </svg>
            <HuffmanTree node={node.children[0]} isRoot={false} path={path + "0"} showLabels={showLabels} />
          </div>
          <div className="child-wrapper">
            <span className="branch-label right-label">1</span>
            <svg className="drop-arrow" width="10" height="24">
              <line x1="5" y1="0" x2="5" y2="21" stroke="#64748b" strokeWidth="2" markerEnd="url(#arrowhead)" />
            </svg>
            <HuffmanTree node={node.children[1]} isRoot={false} path={path + "1"} showLabels={showLabels} />
          </div>
        </div>
      )}
    </div>
  );
};

function App() {
  const [wasmModule, setWasmModule] = useState(null);
  const [simulator, setSimulator] = useState(null);
  const [isDone, setIsDone] = useState(false);
  const [forestState, setForestState] = useState([]);

  // state variables for history tracking for prev button
  const [history, setHistory] = useState([]);
  const [currentStep, setCurrentStep] = useState(0);

  const [showLabels, setShowLabels] = useState(false);

  const [inputChars, setInputChars] = useState("a, b, c, d, e, f, g");
  const [inputFreqs, setInputFreqs] = useState("13, 2, 17, 5, 11, 3, 7");

  // Loading the WASM module
  useEffect(() => {
    if (window.HuffmanCodingModule) {
      window.HuffmanCodingModule().then((module) => {
        setWasmModule(module);
      });
    }
  }, []);

  // Helper to fetch the latest state from C++ and update React
  const syncStateFromWasm = (simInstance) => {
    // Calls C++ getTreesJSON()
    const jsonStr = simInstance.getTreesJSON();
    const parsedForest = JSON.parse(jsonStr);

    setForestState(parsedForest);
    setIsDone(simInstance.getIsDone()); // Calls C++ getIsDone()
  };

  const handleStartSimulation = () => {
    if (!wasmModule) return;
    if (simulator) simulator.delete(); // Prevent memory leaks -> if a simulator already exists in WASM memory, it is deleted

    // Reset UI states
    setShowLabels(false);
    setIsDone(false);

    const charsArray = inputChars.split(',').map(s => s.trim());
    const freqsArray = inputFreqs.split(',').map(s => parseInt(s.trim(), 10));

    // Allocates C++ vectors in WASM memory
    const vecChars = new wasmModule.VectorString();
    const vecFreqs = new wasmModule.VectorInt();

    charsArray.forEach(c => vecChars.push_back(c));
    freqsArray.forEach(f => vecFreqs.push_back(f));

    const sim = new wasmModule.HuffmanSimulator(vecChars, vecFreqs); // runs the constructor of HuffmanSimulator in wasm_bindings.cpp
    setSimulator(sim);

    const jsonStr = sim.getTreesJSON();
    const parsedForest = JSON.parse(jsonStr);

    setHistory([parsedForest]);
    setCurrentStep(0);
    setForestState(parsedForest);
    setIsDone(sim.getIsDone());

    // Freeing the dynamically allocated C++ vectors from WASM memory now that the simulator has consumed them
    vecChars.delete();
    vecFreqs.delete();
  };

  // function to handle Prev (>)
  const handleNext = () => {
    // If we are just viewing history, step forward in the cached array
    if (currentStep < history.length - 1) {
      const nextIndex = currentStep + 1;
      setCurrentStep(nextIndex);
      setForestState(history[nextIndex]);
      setIsDone(history[nextIndex].length <= 1);
    }
    // Otherwise, if we are at the end -> asking WASM to compute the next step
    else if (simulator && !isDone) {
      simulator.stepForward();
      const jsonStr = simulator.getTreesJSON();
      const parsedForest = JSON.parse(jsonStr);

      const newHistory = [...history, parsedForest];
      setHistory(newHistory);
      setCurrentStep(newHistory.length - 1);
      setForestState(parsedForest);
      setIsDone(simulator.getIsDone());
    }
  };

  // function to handle Prev (<)
  const handlePrev = () => {
    if (currentStep > 0) {
      const prevIndex = currentStep - 1;
      setCurrentStep(prevIndex);
      setForestState(history[prevIndex]);
      // If we go backward, we are naturally not on the final completed tree state
      setIsDone(history[prevIndex].length <= 1);
      setShowLabels(false); // Hide final labels if we move backward
    }
  };

  const handleStepForward = () => {
    if (simulator && !isDone) {
      simulator.stepForward(); // Executing one iteration of the C++ logic
      syncStateFromWasm(simulator);
    }
  };

  return (
    <div className="app-theme">
      <svg width="0" height="0" style={{ position: 'absolute' }}>
        <defs>
          <marker id="arrowhead" viewBox="0 0 10 10" refX="5" refY="5" markerWidth="5" markerHeight="5" orient="auto-start-reverse">
            <path d="M 0 0 L 10 5 L 0 10 z" fill="#64748b" />
          </marker>
        </defs>
      </svg>

      <div className="container">
        <header className="header">
          <h1 className="title">Huffman Coding Tree Simulator</h1>
        </header>

        {!wasmModule ? (
          <div className="loading-container">
            <div className="spinner"></div>
            <p className="loading-text">Booting WebAssembly Core...</p>
          </div>
        ) : (
          <div className="main-layout">

            <div className="panel control-panel">
              <div className="inputs-wrapper">
                <div className="input-group">
                  <label>Characters</label>
                  <input value={inputChars} onChange={e => setInputChars(e.target.value)} spellCheck="false" />
                </div>
                <div className="input-group">
                  <label>Frequencies</label>
                  <input value={inputFreqs} onChange={e => setInputFreqs(e.target.value)} spellCheck="false" />
                </div>
              </div>

              <div className="button-group">
                <button className="btn btn-init" onClick={handleStartSimulation}>
                  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M21 12a9 9 0 1 1-9-9c2.52 0 4.93 1 6.74 2.74L21 8" /><path d="M21 3v5h-5" /></svg>
                  Min Heapify
                </button>

                {!isDone ? (
                  <button className="btn btn-step" onClick={handleStepForward} disabled={!simulator}>
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="m9 18 6-6-6-6" /></svg>
                    Extract & Merge
                  </button>
                ) : (
                  <button
                    className="btn btn-step"
                    style={{ backgroundColor: showLabels ? '#ff9500' : '#ffaa00' }}
                    onClick={() => setShowLabels(true)}
                    disabled={showLabels}
                  >
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M20.59 13.41l-7.17 7.17a2 2 0 0 1-2.83 0L2 12V2h10l8.59 8.59a2 2 0 0 1 0 2.82z"></path><line x1="7" y1="7" x2="7.01" y2="7"></line></svg>
                    {showLabels ? 'Labels Added' : 'Add Label'}
                  </button>
                )}
              </div>
            </div>

            <div className="panel visualizer-panel">
              <div className="panel-header">
                <h3 className="panel-title">Simulation of the Tree</h3>

                <div className="navigation-controls">
                  <button
                    className="nav-btn"
                    onClick={handlePrev}
                    disabled={currentStep <= 0}
                    title="Previous State"
                  >
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><polyline points="15 18 9 12 15 6"></polyline></svg>
                  </button>
                  <button
                    className="nav-btn"
                    onClick={handleNext}
                    disabled={isDone && currentStep === history.length - 1}
                    title="Next State"
                  >
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><polyline points="9 18 15 12 9 6"></polyline></svg>
                  </button>
                </div>

              </div>

              <div className="canvas">
                <div className="heap-array-row">
                  {forestState.map((treeRoot) => (
                    <motion.div
                      key={`${treeRoot.char}-${treeRoot.freq}`}
                      layout
                      transition={{ type: "spring", bounce: 0, duration: 0.5 }}
                      className="root-wrapper"
                    >
                      <HuffmanTree node={treeRoot} isRoot={true} showLabels={showLabels} />
                    </motion.div>
                  ))}
                </div>
              </div>

            </div>
          </div>
        )}
      </div>
    </div>
  );
}

export default App;