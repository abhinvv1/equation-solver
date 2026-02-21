import { useState, useEffect } from "react";
import Tree from "react-d3-tree";
import "./App.css";

function App() {
  const [equation, setEquation] = useState("2 * (x + 5) = 20");
  const [result, setResult] = useState("");
  const [astData, setAstData] = useState(null);
  const [solverModule, setSolverModule] = useState(null);

  // Load the WebAssembly module on mount
  useEffect(() => {
    const loadWasm = async () => {
      // createSolver is the global function exported by Emscripten
      const module = await window.createSolver();
      setSolverModule(module);
    };
    loadWasm();
  }, []);

  const handleSolve = () => {
    if (!solverModule) return;

    // Call the C++ function directly from JavaScript!
    const jsonString = solverModule.solveFromJS(equation);
    const data = JSON.parse(jsonString);

    if (data.error) {
      setResult(`Error: ${data.error}`);
      setAstData(null);
    } else {
      setResult(data.result);
      setAstData(data.ast);
    }
  };

  return (
    <div style={{ padding: "20px", fontFamily: "sans-serif" }}>
      <h1>AST Equation Solver (C++ WebAssembly)</h1>

      <div style={{ marginBottom: "20px" }}>
        <input
          type="text"
          value={equation}
          onChange={(e) => setEquation(e.target.value)}
          style={{ padding: "10px", fontSize: "18px", width: "300px" }}
        />
        <button
          onClick={handleSolve}
          style={{ padding: "10px 20px", fontSize: "18px", marginLeft: "10px" }}
        >
          Solve
        </button>
      </div>

      <h2>
        Result: <span style={{ color: "blue" }}>{result}</span>
      </h2>

      {astData && (
        <div
          style={{
            width: "100%",
            height: "500px",
            border: "1px solid #ccc",
            marginTop: "20px",
          }}
        >
          {/* Automatically draws the interactive tree! */}
          <Tree
            data={astData}
            orientation="vertical"
            pathFunc="step"
            translate={{ x: 300, y: 50 }}
          />
        </div>
      )}
    </div>
  );
}

export default App;
