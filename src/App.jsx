import { useState, useEffect } from "react";
import Tree from "react-d3-tree";
import "./App.css";

const renderCustomNodeElement = ({ nodeDatum }) => (
  <g>
    {/* Node Background Box */}
    <rect
      width="60"
      height="40"
      x="-30"
      y="-20"
      rx="8"
      fill="#ffffff"
      stroke="#3b82f6"
      strokeWidth="2"
      className="node-rect"
    />
    {/* Node Text */}
    <text
      fill="#1e293b"
      strokeWidth="1"
      x="0"
      y="5"
      textAnchor="middle"
      style={{ fontSize: "16px", fontWeight: "600" }}
    >
      {nodeDatum.name}
    </text>
  </g>
);

function App() {
  const [equation, setEquation] = useState("2 * (x + 5) = 20");
  const [result, setResult] = useState("");
  const [error, setError] = useState("");
  const [astData, setAstData] = useState(null);
  const [solverModule, setSolverModule] = useState(null);

  useEffect(() => {
    const loadWasm = async () => {
      if (window.createSolver) {
        const module = await window.createSolver();
        setSolverModule(module);
      }
    };
    loadWasm();
  }, []);

  const handleSolve = () => {
    if (!solverModule) return;
    setError("");

    const jsonString = solverModule.solveFromJS(equation);
    const data = JSON.parse(jsonString);

    if (data.error) {
      setError(data.error);
      setAstData(null);
      setResult("");
    } else {
      setResult(data.result);
      setAstData(data.ast);
    }
  };

  return (
    <div className="app-container">
      <div className="header">
        <h1>AST Equation Solver</h1>
        <p>C++ WebAssembly Engine</p>
      </div>

      <div className="input-card">
        <input
          type="text"
          value={equation}
          onChange={(e) => setEquation(e.target.value)}
          onKeyDown={(e) => e.key === "Enter" && handleSolve()}
          className="styled-input"
          placeholder="Enter a linear equation..."
        />
        <button onClick={handleSolve} className="styled-button">
          Solve
        </button>
      </div>

      <div className="result-area">
        {error && <div className="error-text">Error: {error}</div>}
        {result && !error && (
          <div className="result-text">
            Output: <span className="result-value">{result}</span>
          </div>
        )}
      </div>

      {astData && (
        <div className="tree-card">
          <Tree
            data={astData}
            orientation="vertical"
            pathFunc="step"
            translate={{ x: 480, y: 50 }}
            renderCustomNodeElement={renderCustomNodeElement}
            separation={{ siblings: 1.5, nonSiblings: 2 }}
            zoomable={true}
          />
        </div>
      )}
    </div>
  );
}

export default App;
