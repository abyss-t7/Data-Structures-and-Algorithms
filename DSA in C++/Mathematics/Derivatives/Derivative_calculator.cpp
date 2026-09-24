/*
 *  Derivative Calculator 

 *  A real SYMBOLIC differentiation engine (not numerical approximation).
 *  Handles:
 *      - Ordinary high-order derivatives      d^n f / dx^n
 *      - Partial derivatives                  df/dx, df/dy, ...
 *      - Higher-order mixed partial derivs    d^3 f / dx^2 dy , etc.
 *
 *  HOW IT WORKS
 *  ------------
 *  1. Tokenizer  -> turns the input string into tokens.
 *  2. Parser     -> recursive-descent parser builds an Abstract Syntax
 *                   Tree (AST) representing the expression.
 *  3. diff()     -> recursively applies calculus rules (sum, product,
 *                   quotient, chain, power) to build the derivative AST.
 *  4. simplify() -> cleans up the resulting AST (removes *0, *1, +0, etc.)
 *  5. toString() -> converts the AST back into a readable math string.
 *  6. eval()     -> numerically evaluates any AST at a given point.
 *
 *  SUPPORTED SYNTAX
 *  -----------------
 *    Operators : +  -  *  /  ^        (use * explicitly, e.g. 2*x not 2x)
 *    Functions : sin(...) cos(...) tan(...) exp(...) ln(...) sqrt(...)
 *    Variables : any letter/word not matching a function name, e.g. x, y, t
 *    Examples  : x^3*y - sin(x*y) + exp(y)/x - ln(x) + sqrt(x*y)
 *
 *  Compile:  g++ -std=c++17 -O2 -o derivative_calculator derivative_calculator.cpp
 *  Run:      ./derivative_calculator
 * ============================================================================
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <cmath>
#include <cctype>
#include <stdexcept>
#include <algorithm>

using namespace std;

// ---------------------------------------------------------------------------
// 1. AST NODE DEFINITION
// ---------------------------------------------------------------------------
enum class Op {
    Num, Var,                    // leaves
    Add, Sub, Mul, Div, Pow,     // binary
    Neg, Sin, Cos, Tan, Exp, Ln, Sqrt   // unary
};

struct Node {
    Op op;
    double num = 0.0;
    string var;
    shared_ptr<Node> a, b;       // a = left/operand, b = right (binary only)
};
using NodePtr = shared_ptr<Node>;

// --- convenience constructors ----------------------------------------------
NodePtr mkNum(double v) { auto n = make_shared<Node>(); n->op = Op::Num; n->num = v; return n; }
NodePtr mkVar(const string& name) { auto n = make_shared<Node>(); n->op = Op::Var; n->var = name; return n; }
NodePtr mkBin(Op op, NodePtr a, NodePtr b) { auto n = make_shared<Node>(); n->op = op; n->a = a; n->b = b; return n; }
NodePtr mkUn(Op op, NodePtr a) { auto n = make_shared<Node>(); n->op = op; n->a = a; return n; }

// ---------------------------------------------------------------------------
// 2. TOKENIZER
// ---------------------------------------------------------------------------
enum class TType { Num, Ident, Plus, Minus, Star, Slash, Caret, LParen, RParen, End };

struct Token { TType type; string text; double val = 0.0; };

vector<Token> tokenize(const string& s) {
    vector<Token> tokens;
    size_t i = 0, n = s.size();
    while (i < n) {
        char c = s[i];
        if (isspace((unsigned char)c)) { i++; continue; }
        if (isdigit((unsigned char)c) || c == '.') {
            size_t start = i;
            bool seenDot = false;
            while (i < n && (isdigit((unsigned char)s[i]) || (s[i] == '.' && !seenDot))) {
                if (s[i] == '.') seenDot = true;
                i++;
            }
            string numStr = s.substr(start, i - start);
            tokens.push_back({TType::Num, numStr, stod(numStr)});
            continue;
        }
        if (isalpha((unsigned char)c) || c == '_') {
            size_t start = i;
            while (i < n && (isalnum((unsigned char)s[i]) || s[i] == '_')) i++;
            tokens.push_back({TType::Ident, s.substr(start, i - start), 0.0});
            continue;
        }
        switch (c) {
            case '+': tokens.push_back({TType::Plus, "+"}); break;
            case '-': tokens.push_back({TType::Minus, "-"}); break;
            case '*': tokens.push_back({TType::Star, "*"}); break;
            case '/': tokens.push_back({TType::Slash, "/"}); break;
            case '^': tokens.push_back({TType::Caret, "^"}); break;
            case '(': tokens.push_back({TType::LParen, "("}); break;
            case ')': tokens.push_back({TType::RParen, ")"}); break;
            default:
                throw runtime_error(string("Unexpected character '") + c + "' in expression");
        }
        i++;
    }
    tokens.push_back({TType::End, ""});
    return tokens;
}

// ---------------------------------------------------------------------------
// 3. PARSER  (recursive descent)
//
//   expr   -> term (('+' | '-') term)*
//   term   -> unary (('*' | '/') unary)*
//   unary  -> '-' unary | power
//   power  -> primary ('^' unary)?          (right-associative, allows x^-2)
//   primary-> NUM | IDENT ['(' expr ')'] | '(' expr ')'
// ---------------------------------------------------------------------------
static const set<string> FUNC_NAMES = {"sin", "cos", "tan", "exp", "ln", "sqrt"};

Op funcNameToOp(const string& name) {
    if (name == "sin") return Op::Sin;
    if (name == "cos") return Op::Cos;
    if (name == "tan") return Op::Tan;
    if (name == "exp") return Op::Exp;
    if (name == "ln")  return Op::Ln;
    if (name == "sqrt")return Op::Sqrt;
    throw runtime_error("Unknown function: " + name);
}

class Parser {
public:
    explicit Parser(const string& src) : tokens(tokenize(src)), pos(0) {}

    NodePtr parse() {
        NodePtr result = parseExpr();
        expect(TType::End, "end of expression (check for a stray character)");
        return result;
    }

private:
    vector<Token> tokens;
    size_t pos;

    const Token& peek() { return tokens[pos]; }
    Token advance() { return tokens[pos++]; }
    void expect(TType t, const string& what) {
        if (peek().type != t)
            throw runtime_error("Expected " + what + " but found '" + peek().text + "'");
        advance();
    }

    NodePtr parseExpr() {
        NodePtr node = parseTerm();
        while (peek().type == TType::Plus || peek().type == TType::Minus) {
            bool isPlus = peek().type == TType::Plus;
            advance();
            NodePtr rhs = parseTerm();
            node = mkBin(isPlus ? Op::Add : Op::Sub, node, rhs);
        }
        return node;
    }

    NodePtr parseTerm() {
        NodePtr node = parseUnary();
        while (peek().type == TType::Star || peek().type == TType::Slash) {
            bool isMul = peek().type == TType::Star;
            advance();
            NodePtr rhs = parseUnary();
            node = mkBin(isMul ? Op::Mul : Op::Div, node, rhs);
        }
        return node;
    }

    NodePtr parseUnary() {
        if (peek().type == TType::Minus) {
            advance();
            return mkUn(Op::Neg, parseUnary());
        }
        return parsePower();
    }

    NodePtr parsePower() {
        NodePtr base = parsePrimary();
        if (peek().type == TType::Caret) {
            advance();
            NodePtr exponent = parseUnary();   // right-assoc; allows x^-2, x^2^y
            return mkBin(Op::Pow, base, exponent);
        }
        return base;
    }

    NodePtr parsePrimary() {
        Token t = peek();
        if (t.type == TType::Num) { advance(); return mkNum(t.val); }
        if (t.type == TType::Ident) {
            advance();
            if (FUNC_NAMES.count(t.text) && peek().type == TType::LParen) {
                advance(); // consume '('
                NodePtr arg = parseExpr();
                expect(TType::RParen, "')' to close " + t.text + "(...)");
                return mkUn(funcNameToOp(t.text), arg);
            }
            return mkVar(t.text);
        }
        if (t.type == TType::LParen) {
            advance();
            NodePtr inner = parseExpr();
            expect(TType::RParen, "')'");
            return inner;
        }
        throw runtime_error("Unexpected token '" + t.text + "' — expected a number, variable, function, or '('");
    }
};

// ---------------------------------------------------------------------------
// 4. SYMBOLIC DIFFERENTIATION
//    Subtrees that don't change (like the 'b' in a product rule term) are
//    safely reused as shared_ptr — nodes are never mutated in place.
// ---------------------------------------------------------------------------
NodePtr diff(const NodePtr& n, const string& var) {
    switch (n->op) {
        case Op::Num: return mkNum(0);
        case Op::Var: return mkNum(n->var == var ? 1.0 : 0.0);

        case Op::Add: return mkBin(Op::Add, diff(n->a, var), diff(n->b, var));
        case Op::Sub: return mkBin(Op::Sub, diff(n->a, var), diff(n->b, var));

        case Op::Mul: // product rule: (uv)' = u'v + uv'
            return mkBin(Op::Add,
                          mkBin(Op::Mul, diff(n->a, var), n->b),
                          mkBin(Op::Mul, n->a, diff(n->b, var)));

        case Op::Div: // quotient rule: (u/v)' = (u'v - uv') / v^2
            return mkBin(Op::Div,
                          mkBin(Op::Sub,
                                mkBin(Op::Mul, diff(n->a, var), n->b),
                                mkBin(Op::Mul, n->a, diff(n->b, var))),
                          mkBin(Op::Pow, n->b, mkNum(2)));

        case Op::Pow: {
            bool expConst  = (n->b->op == Op::Num);
            bool baseConst = (n->a->op == Op::Num);
            if (expConst) {
                // power rule + chain rule: d/dx[u^c] = c * u^(c-1) * u'
                return mkBin(Op::Mul,
                              mkBin(Op::Mul, mkNum(n->b->num), mkBin(Op::Pow, n->a, mkNum(n->b->num - 1))),
                              diff(n->a, var));
            } else if (baseConst) {
                // d/dx[c^v] = c^v * ln(c) * v'
                return mkBin(Op::Mul, mkBin(Op::Mul, n, mkUn(Op::Ln, n->a)), diff(n->b, var));
            } else {
                // general case: d/dx[u^v] = u^v * (v'*ln(u) + v*u'/u)
                return mkBin(Op::Mul, n,
                              mkBin(Op::Add,
                                    mkBin(Op::Mul, diff(n->b, var), mkUn(Op::Ln, n->a)),
                                    mkBin(Op::Mul, n->b, mkBin(Op::Div, diff(n->a, var), n->a))));
            }
        }

        case Op::Neg: return mkUn(Op::Neg, diff(n->a, var));

        case Op::Sin: return mkBin(Op::Mul, mkUn(Op::Cos, n->a), diff(n->a, var));
        case Op::Cos: return mkUn(Op::Neg, mkBin(Op::Mul, mkUn(Op::Sin, n->a), diff(n->a, var)));
        case Op::Tan: return mkBin(Op::Div, diff(n->a, var), mkBin(Op::Pow, mkUn(Op::Cos, n->a), mkNum(2)));
        case Op::Exp: return mkBin(Op::Mul, mkUn(Op::Exp, n->a), diff(n->a, var));
        case Op::Ln:  return mkBin(Op::Div, diff(n->a, var), n->a);
        case Op::Sqrt:return mkBin(Op::Div, diff(n->a, var), mkBin(Op::Mul, mkNum(2), mkUn(Op::Sqrt, n->a)));
    }
    throw runtime_error("diff(): unhandled node type");
}

// ---------------------------------------------------------------------------
// 5. SIMPLIFICATION  (bottom-up; children are simplified before the parent)
// ---------------------------------------------------------------------------
NodePtr simplify(const NodePtr& n) {
    if (!n) return n;
    switch (n->op) {
        case Op::Num: case Op::Var: return n;

        case Op::Neg: {
            NodePtr a = simplify(n->a);
            if (a->op == Op::Num) return mkNum(-a->num);
            if (a->op == Op::Neg) return a->a;               // -(-x) = x
            return mkUn(Op::Neg, a);
        }
        case Op::Add: {
            NodePtr a = simplify(n->a), b = simplify(n->b);
            if (a->op == Op::Num && b->op == Op::Num) return mkNum(a->num + b->num);
            if (a->op == Op::Num && a->num == 0) return b;
            if (b->op == Op::Num && b->num == 0) return a;
            // collapse chained constants: c1 + (c2 + x) -> (c1+c2) + x
            if (a->op == Op::Num && b->op == Op::Add && b->a->op == Op::Num)
                return simplify(mkBin(Op::Add, mkNum(a->num + b->a->num), b->b));
            if (b->op == Op::Num && a->op == Op::Add && a->a->op == Op::Num)
                return simplify(mkBin(Op::Add, mkNum(b->num + a->a->num), a->b));
            return mkBin(Op::Add, a, b);
        }
        case Op::Sub: {
            NodePtr a = simplify(n->a), b = simplify(n->b);
            if (a->op == Op::Num && b->op == Op::Num) return mkNum(a->num - b->num);
            if (b->op == Op::Num && b->num == 0) return a;
            if (a->op == Op::Num && a->num == 0) return simplify(mkUn(Op::Neg, b));
            return mkBin(Op::Sub, a, b);
        }
        case Op::Mul: {
            NodePtr a = simplify(n->a), b = simplify(n->b);
            if ((a->op == Op::Num && a->num == 0) || (b->op == Op::Num && b->num == 0)) return mkNum(0);
            if (a->op == Op::Num && b->op == Op::Num) return mkNum(a->num * b->num);
            if (a->op == Op::Num && a->num == 1) return b;
            if (b->op == Op::Num && b->num == 1) return a;
            if (a->op == Op::Num && a->num == -1) return simplify(mkUn(Op::Neg, b));
            if (b->op == Op::Num && b->num == -1) return simplify(mkUn(Op::Neg, a));
            // collapse chained constants: c1 * (c2 * x) -> (c1*c2) * x  (and symmetric cases)
            if (a->op == Op::Num && b->op == Op::Mul && b->a->op == Op::Num)
                return simplify(mkBin(Op::Mul, mkNum(a->num * b->a->num), b->b));
            if (a->op == Op::Num && b->op == Op::Mul && b->b->op == Op::Num)
                return simplify(mkBin(Op::Mul, mkNum(a->num * b->b->num), b->a));
            if (b->op == Op::Num && a->op == Op::Mul && a->a->op == Op::Num)
                return simplify(mkBin(Op::Mul, mkNum(b->num * a->a->num), a->b));
            if (b->op == Op::Num && a->op == Op::Mul && a->b->op == Op::Num)
                return simplify(mkBin(Op::Mul, mkNum(b->num * a->b->num), a->a));
            return mkBin(Op::Mul, a, b);
        }
        case Op::Div: {
            NodePtr a = simplify(n->a), b = simplify(n->b);
            if (a->op == Op::Num && a->num == 0) return mkNum(0);
            if (b->op == Op::Num && b->num == 1) return a;
            if (a->op == Op::Num && b->op == Op::Num && b->num != 0) return mkNum(a->num / b->num);
            return mkBin(Op::Div, a, b);
        }
        case Op::Pow: {
            NodePtr a = simplify(n->a), b = simplify(n->b);
            if (b->op == Op::Num && b->num == 1) return a;
            if (b->op == Op::Num && b->num == 0) return mkNum(1);
            if (a->op == Op::Num && a->num == 1) return mkNum(1);
            if (a->op == Op::Num && a->num == 0 && b->op == Op::Num && b->num > 0) return mkNum(0);
            if (a->op == Op::Num && b->op == Op::Num) return mkNum(pow(a->num, b->num));
            return mkBin(Op::Pow, a, b);
        }
        case Op::Sin: case Op::Cos: case Op::Tan:
        case Op::Exp: case Op::Ln:  case Op::Sqrt: {
            NodePtr a = simplify(n->a);
            return mkUn(n->op, a);
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
// 6. PRETTY PRINTING  (adds parentheses only where required, and rewrites
//    "a + (-b)" as "a - b" for readability)
// ---------------------------------------------------------------------------
string toStringNode(const NodePtr& n);

int precedence(Op op) {
    switch (op) {
        case Op::Add: case Op::Sub: return 1;
        case Op::Mul: case Op::Div: return 2;
        case Op::Neg: return 3;
        case Op::Pow: return 4;
        default: return 5; // Num, Var, functions
    }
}

string wrapIfNeeded(const NodePtr& child, Op parentOp, bool isRightOperand) {
    int pp = precedence(parentOp), cp = precedence(child->op);
    bool needParen = false;
    if (cp < pp) needParen = true;
    else if (cp == pp) {
        if ((parentOp == Op::Sub || parentOp == Op::Div) && isRightOperand) needParen = true;
        if (parentOp == Op::Pow && !isRightOperand && child->op == Op::Pow) needParen = true;
    }
    string s = toStringNode(child);
    return needParen ? ("(" + s + ")") : s;
}

string numToStr(double v) {
    if (fabs(v - llround(v)) < 1e-9) return to_string(llround(v));
    ostringstream oss; oss << v; return oss.str();
}

string funcName(Op op) {
    switch (op) {
        case Op::Sin: return "sin"; case Op::Cos: return "cos"; case Op::Tan: return "tan";
        case Op::Exp: return "exp"; case Op::Ln: return "ln"; case Op::Sqrt: return "sqrt";
        default: return "?";
    }
}

string toStringNode(const NodePtr& n) {
    switch (n->op) {
        case Op::Num: return numToStr(n->num);
        case Op::Var: return n->var;
        case Op::Neg: return "-" + wrapIfNeeded(n->a, Op::Neg, false);
        case Op::Add: {
            if (n->b->op == Op::Neg)
                return wrapIfNeeded(n->a, Op::Sub, false) + " - " + wrapIfNeeded(n->b->a, Op::Sub, true);
            if (n->b->op == Op::Num && n->b->num < 0)
                return wrapIfNeeded(n->a, Op::Sub, false) + " - " + numToStr(-n->b->num);
            return wrapIfNeeded(n->a, Op::Add, false) + " + " + wrapIfNeeded(n->b, Op::Add, true);
        }
        case Op::Sub: {
            if (n->b->op == Op::Neg)
                return wrapIfNeeded(n->a, Op::Add, false) + " + " + wrapIfNeeded(n->b->a, Op::Add, true);
            if (n->b->op == Op::Num && n->b->num < 0)
                return wrapIfNeeded(n->a, Op::Add, false) + " + " + numToStr(-n->b->num);
            return wrapIfNeeded(n->a, Op::Sub, false) + " - " + wrapIfNeeded(n->b, Op::Sub, true);
        }
        case Op::Mul: return wrapIfNeeded(n->a, Op::Mul, false) + "*" + wrapIfNeeded(n->b, Op::Mul, true);
        case Op::Div: return wrapIfNeeded(n->a, Op::Div, false) + "/" + wrapIfNeeded(n->b, Op::Div, true);
        case Op::Pow: return wrapIfNeeded(n->a, Op::Pow, false) + "^" + wrapIfNeeded(n->b, Op::Pow, true);
        default: return funcName(n->op) + "(" + toStringNode(n->a) + ")";
    }
}

// ---------------------------------------------------------------------------
// 7. NUMERIC EVALUATION
// ---------------------------------------------------------------------------
double evalNode(const NodePtr& n, const map<string, double>& vals) {
    switch (n->op) {
        case Op::Num: return n->num;
        case Op::Var: {
            auto it = vals.find(n->var);
            if (it == vals.end()) throw runtime_error("No value given for variable '" + n->var + "'");
            return it->second;
        }
        case Op::Add: return evalNode(n->a, vals) + evalNode(n->b, vals);
        case Op::Sub: return evalNode(n->a, vals) - evalNode(n->b, vals);
        case Op::Mul: return evalNode(n->a, vals) * evalNode(n->b, vals);
        case Op::Div: return evalNode(n->a, vals) / evalNode(n->b, vals);
        case Op::Pow: return pow(evalNode(n->a, vals), evalNode(n->b, vals));
        case Op::Neg: return -evalNode(n->a, vals);
        case Op::Sin: return sin(evalNode(n->a, vals));
        case Op::Cos: return cos(evalNode(n->a, vals));
        case Op::Tan: return tan(evalNode(n->a, vals));
        case Op::Exp: return exp(evalNode(n->a, vals));
        case Op::Ln:  return log(evalNode(n->a, vals));
        case Op::Sqrt:return sqrt(evalNode(n->a, vals));
    }
    throw runtime_error("eval(): unhandled node type");
}

void collectVars(const NodePtr& n, set<string>& vars) {
    if (!n) return;
    if (n->op == Op::Var) vars.insert(n->var);
    collectVars(n->a, vars);
    collectVars(n->b, vars);
}

// ---------------------------------------------------------------------------
// small helpers for the CLI
// ---------------------------------------------------------------------------
vector<string> splitWords(const string& s) {
    vector<string> out; istringstream iss(s); string w;
    while (iss >> w) out.push_back(w);
    return out;
}

int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        string line; getline(cin, line);
        try {
            size_t idx; int v = stoi(line, &idx);
            if (idx == line.size() && v > 0) return v;
        } catch (...) {}
        cout << "  Please enter a positive whole number.\n";
    }
}

double readDouble(const string& prompt) {
    while (true) {
        cout << prompt;
        string line; getline(cin, line);
        try { size_t idx; double v = stod(line, &idx); if (idx == line.size()) return v; }
        catch (...) {}
        cout << "  Please enter a valid number.\n";
    }
}

void printVarSet(const set<string>& vars) {
    cout << "  Variables found: ";
    if (vars.empty()) { cout << "(none — constant expression)\n"; return; }
    bool first = true;
    for (auto& v : vars) { if (!first) cout << ", "; cout << v; first = false; }
    cout << "\n";
}

// ---------------------------------------------------------------------------
// 8. MAIN — interactive CLI
// ---------------------------------------------------------------------------
int main() {
    cout << "========================================================\n";
    cout << " Derivative Calculator  (ordinary, partial, mixed-partial)\n";
    cout << "========================================================\n";
    cout << "Syntax: + - * / ^   functions: sin cos tan exp ln sqrt\n";
    cout << "Use '*' explicitly for multiplication (e.g. 2*x, not 2x).\n";
    cout << "Example: x^3*y - sin(x*y) + exp(y)/x - ln(x) + sqrt(x*y)\n\n";

    while (true) {
        cout << "Enter a function f(...)  [or type 'quit' to exit]:\n> ";
        string line;
        if (!getline(cin, line)) break;
        string trimmed = line;
        // strip leading/trailing spaces
        size_t s1 = trimmed.find_first_not_of(" \t");
        if (s1 == string::npos) continue;
        size_t s2 = trimmed.find_last_not_of(" \t");
        trimmed = trimmed.substr(s1, s2 - s1 + 1);
        if (trimmed == "quit" || trimmed == "exit") break;

        NodePtr f;
        try {
            Parser parser(trimmed);
            f = simplify(parser.parse());
        } catch (exception& e) {
            cout << "  [Parse error] " << e.what() << "\n\n";
            continue;
        }

        set<string> vars;
        collectVars(f, vars);
        cout << "\n  f = " << toStringNode(f) << "\n";
        printVarSet(vars);
        cout << "\n";

        NodePtr currentResult = f;   // remembers last computed derivative, for evaluation

        bool newFunction = false;
        while (!newFunction) {
            cout << "----------------------------------------------------------------\n";
            cout << "1) High-order derivative w.r.t. ONE variable   (d^n f / dx^n)\n";
            cout << "2) Partial / mixed derivative  (custom variable sequence)\n";
            cout << "3) Evaluate f or the last derivative at a point\n";
            cout << "4) Enter a new function\n";
            cout << "5) Quit\n";
            cout << "Choice: ";
            string choice; getline(cin, choice);

            if (choice == "1") {
                cout << "  Differentiate with respect to which variable? ";
                string v; getline(cin, v);
                int order = readInt("  Order n (e.g. 1, 2, 3, ...): ");
                NodePtr result = f;
                for (int k = 1; k <= order; k++) {
                    result = simplify(diff(result, v));
                    cout << "  d^" << k << "f/d" << v << "^" << k << " = " << toStringNode(result) << "\n";
                }
                currentResult = result;
                cout << "\n  Final answer: d^" << order << "f/d" << v << "^" << order
                     << " = " << toStringNode(result) << "\n\n";

            } else if (choice == "2") {
                cout << "  Enter variables in the order you want to differentiate,\n";
                cout << "  separated by spaces (e.g. 'x y' computes d/dy( d/dx(f) ) ):\n  > ";
                string seqLine; getline(cin, seqLine);
                vector<string> seq = splitWords(seqLine);
                if (seq.empty()) { cout << "  No variables entered.\n\n"; continue; }
                NodePtr result = f;
                for (size_t k = 0; k < seq.size(); k++) {
                    result = simplify(diff(result, seq[k]));
                    cout << "  Step " << (k + 1) << " (d/d" << seq[k] << "): " << toStringNode(result) << "\n";
                }
                // build a tidy notation, e.g. d^3f / dx^2 dy
                map<string,int> mult;
                for (auto& v : seq) mult[v]++;
                ostringstream notation;
                notation << "d^" << seq.size() << "f / ";
                for (auto& [v, c] : mult) {
                    notation << "d" << v;
                    if (c > 1) notation << "^" << c;
                    notation << " ";
                }
                currentResult = result;
                cout << "\n  Final answer: " << notation.str() << "= " << toStringNode(result) << "\n\n";

            } else if (choice == "3") {
                cout << "  Evaluate: (a) original f   (b) last computed derivative? [a/b]: ";
                string which; getline(cin, which);
                NodePtr target = (which == "b") ? currentResult : f;
                set<string> tvars; collectVars(target, tvars);
                map<string, double> vals;
                for (auto& v : tvars)
                    vals[v] = readDouble("    Value of " + v + " = ");
                try {
                    double result = evalNode(target, vals);
                    cout << "  Result: " << toStringNode(target) << "  =  " << result << "\n\n";
                } catch (exception& e) {
                    cout << "  [Evaluation error] " << e.what() << "\n\n";
                }

            } else if (choice == "4") {
                newFunction = true;
            } else if (choice == "5") {
                return 0;
            } else {
                cout << "  Please choose 1-5.\n";
            }
        }
    }

    cout << "Goodbye!\n";
    return 0;
}