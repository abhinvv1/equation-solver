#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cctype>
#include <map>
#include <cmath>
#include <stdexcept>
#include <emscripten/bind.h>

// ==========================================
// 1. DYNAMIC POLYNOMIAL REPRESENTATION
// ==========================================
struct Poly {
    std::map<int, double> terms;

    void clean() {
        for (auto it = terms.begin(); it != terms.end(); ) {
            if (std::abs(it->second) < 1e-9) it = terms.erase(it);
            else ++it;
        }
    }
};

enum class TokenType { 
    NUMBER, VARIABLE, PLUS, MINUS, MUL, DIV, POWER, EQUALS, LPAREN, RPAREN, END_OF_FILE 
};

struct Token { 
    TokenType type; 
    std::string value; 
};

// ==========================================
// 2. LEXER (Tokenization)
// ==========================================
class Lexer {
    std::string text;
    size_t pos;
    
    char currentChar() { return pos < text.length() ? text[pos] : '\0'; }
    void advance() { pos++; }

public:
    Lexer(std::string input) : text(input), pos(0) {}
    
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (currentChar() != '\0') {
            if (isspace(currentChar())) {
                advance();
            } else if (isdigit(currentChar()) || currentChar() == '.') {
                std::string numStr;
                while (isdigit(currentChar()) || currentChar() == '.') {
                    numStr += currentChar(); 
                    advance();
                }
                tokens.push_back({TokenType::NUMBER, numStr});
            } else if (isalpha(currentChar())) {
                tokens.push_back({TokenType::VARIABLE, std::string(1, currentChar())}); 
                advance();
            } else {
                switch (currentChar()) {
                    case '+': tokens.push_back({TokenType::PLUS, "+"}); break;
                    case '-': tokens.push_back({TokenType::MINUS, "-"}); break;
                    case '*': tokens.push_back({TokenType::MUL, "*"}); break;
                    case '/': tokens.push_back({TokenType::DIV, "/"}); break;
                    case '^': tokens.push_back({TokenType::POWER, "^"}); break;
                    case '=': tokens.push_back({TokenType::EQUALS, "="}); break;
                    case '(': tokens.push_back({TokenType::LPAREN, "("}); break;
                    case ')': tokens.push_back({TokenType::RPAREN, ")"}); break;
                    default: throw std::runtime_error("Unknown character in input");
                }
                advance();
            }
        }
        tokens.push_back({TokenType::END_OF_FILE, ""});
        return tokens;
    }
};

// ==========================================
// 3. AST NODES (With JSON Export)
// ==========================================
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual Poly evaluate() = 0;
    virtual std::string toJSON() const = 0; 
};

class NumberNode : public ASTNode {
    double value;
public:
    NumberNode(double val) : value(val) {}
    Poly evaluate() override { Poly p; p.terms[0] = value; return p; }
    
    std::string toJSON() const override {
        std::string valStr = std::to_string(value);
        valStr.erase(valStr.find_last_not_of('0') + 1, std::string::npos);
        if(valStr.back() == '.') valStr.pop_back();
        return "{\"name\": \"" + valStr + "\"}";
    }
};

class VariableNode : public ASTNode {
public:
    Poly evaluate() override { Poly p; p.terms[1] = 1.0; return p; }
    std::string toJSON() const override { return "{\"name\": \"x\"}"; }
};

class BinaryOpNode : public ASTNode {
    std::unique_ptr<ASTNode> left, right;
    TokenType op;
public:
    BinaryOpNode(std::unique_ptr<ASTNode> l, TokenType o, std::unique_ptr<ASTNode> r)
        : left(std::move(l)), right(std::move(r)), op(o) {}

    Poly evaluate() override {
        Poly l = left->evaluate();
        Poly r = right->evaluate();
        Poly result;

        if (op == TokenType::PLUS || op == TokenType::MINUS) {
            result = l;
            for (auto const& [pow, coeff] : r.terms) {
                result.terms[pow] += (op == TokenType::PLUS ? coeff : -coeff);
            }
        } 
        else if (op == TokenType::MUL) {
            for (auto const& [powL, coeffL] : l.terms) {
                for (auto const& [powR, coeffR] : r.terms) {
                    result.terms[powL + powR] += (coeffL * coeffR);
                }
            }
        }
        else if (op == TokenType::POWER) {
            if (r.terms.size() != 1 || r.terms.count(0) == 0) {
                throw std::runtime_error("Exponent must be a constant number");
            }
            int exp = std::round(r.terms[0]);
            result.terms[0] = 1.0; 
            for(int i = 0; i < exp; ++i) { 
                Poly temp;
                for (auto const& [powRes, coeffRes] : result.terms) {
                    for (auto const& [powL, coeffL] : l.terms) {
                        temp.terms[powRes + powL] += (coeffRes * coeffL);
                    }
                }
                result = temp;
            }
        }
        result.clean();
        return result;
    }

    std::string opToString() const {
        switch(op) {
            case TokenType::PLUS: return "+"; case TokenType::MINUS: return "-";
            case TokenType::MUL: return "*"; case TokenType::DIV: return "/";
            case TokenType::POWER: return "^"; case TokenType::EQUALS: return "=";
            default: return "?";
        }
    }

    std::string toJSON() const override {
        return "{\"name\": \"" + opToString() + "\", \"children\": [" + 
               left->toJSON() + ", " + right->toJSON() + "]}";
    }
};

// ==========================================
// 4. PARSER & WEB EXPORT LOGIC
// ==========================================
class Parser {
    std::vector<Token> tokens;
    size_t pos;
    
    Token currentToken() { return tokens[pos]; }
    void eat(TokenType type) { 
        if (currentToken().type == type) pos++; 
        else throw std::runtime_error("Unexpected token syntax");
    }

    std::unique_ptr<ASTNode> parseFactor() {
        Token token = currentToken();
        if (token.type == TokenType::NUMBER) { 
            eat(TokenType::NUMBER); 
            return std::make_unique<NumberNode>(std::stod(token.value)); 
        }
        if (token.type == TokenType::VARIABLE) { 
            eat(TokenType::VARIABLE); 
            return std::make_unique<VariableNode>(); 
        }
        if (token.type == TokenType::LPAREN) {
            eat(TokenType::LPAREN); 
            auto node = parseExpression(); 
            eat(TokenType::RPAREN); 
            return node;
        }
        throw std::runtime_error("Syntax error in factor");
    }

    std::unique_ptr<ASTNode> parsePower() {
        auto node = parseFactor();
        while (currentToken().type == TokenType::POWER) {
            eat(TokenType::POWER);
            node = std::make_unique<BinaryOpNode>(std::move(node), TokenType::POWER, parseFactor());
        }
        return node;
    }

    std::unique_ptr<ASTNode> parseTerm() {
        auto node = parsePower(); 
        while (currentToken().type == TokenType::MUL || currentToken().type == TokenType::DIV) {
            TokenType op = currentToken().type;
            eat(op);
            node = std::make_unique<BinaryOpNode>(std::move(node), op, parsePower());
        }
        return node;
    }

    std::unique_ptr<ASTNode> parseExpression() {
        auto node = parseTerm();
        while (currentToken().type == TokenType::PLUS || currentToken().type == TokenType::MINUS) {
            TokenType op = currentToken().type; 
            eat(op);
            node = std::make_unique<BinaryOpNode>(std::move(node), op, parseTerm());
        }
        return node;
    }

public:
    Parser(std::vector<Token> toks) : tokens(std::move(toks)), pos(0) {}

    std::string solveEquationForWeb() {
        auto leftSide = parseExpression();
        eat(TokenType::EQUALS);
        auto rightSide = parseExpression();

        std::string astJSON = "{\"name\": \"=\", \"children\": [" + leftSide->toJSON() + ", " + rightSide->toJSON() + "]}";

        Poly L = leftSide->evaluate();
        Poly R = rightSide->evaluate();

        Poly finalEq = L;
        for (auto const& [pow, coeff] : R.terms) {
            finalEq.terms[pow] -= coeff;
        }
        finalEq.clean();

        std::string resultText = "";
        if (finalEq.terms.empty()) { 
            resultText = "0 = 0. Infinite solutions."; 
        } else {
            int degree = finalEq.terms.rbegin()->first;
            if (degree == 1) {
                double a = finalEq.terms[1];
                double c = finalEq.terms[0];
                resultText = "x = " + std::to_string(-c / a);
            } else if (degree == 2) {
                double a = finalEq.terms[2], b = finalEq.terms[1], c = finalEq.terms[0];
                double discriminant = b*b - 4*a*c;
                if (discriminant > 0) {
                    resultText = "x1 = " + std::to_string((-b + std::sqrt(discriminant)) / (2*a)) + ", x2 = " + std::to_string((-b - std::sqrt(discriminant)) / (2*a));
                } else if (discriminant == 0) {
                    resultText = "x = " + std::to_string(-b / (2*a));
                } else {
                    resultText = "Complex Roots (no real solution).";
                }
            } else {
                resultText = "Equation degree is " + std::to_string(degree) + ". Exact algebraic solution omitted. Numerical methods required.";
            }
        }

        return "{\"ast\": " + astJSON + ", \"result\": \"" + resultText + "\"}";
    }
};

// ==========================================
// 5. EMSCRIPTEN BINDINGS
// ==========================================

// This function acts as our bridge to JavaScript
std::string solveFromJS(std::string equation) {
    try {
        Lexer lexer(equation);
        Parser parser(lexer.tokenize());
        return parser.solveEquationForWeb();
    } catch (const std::exception& e) {
        return "{\"error\": \"" + std::string(e.what()) + "\"}";
    }
}

// Dummy main is sometimes required by Emscripten to satisfy the linker
int main() { return 0; }

EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("solveFromJS", &solveFromJS);
}