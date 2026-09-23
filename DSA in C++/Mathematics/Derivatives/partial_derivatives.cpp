// Partial, high-order, and mixed partial derivative solver
#include <bits/stdc++.h>
using namespace std;

struct Node;
using NodePtr = shared_ptr<Node>;

enum class Op { Add, Sub, Mul, Div, Pow };
enum class Fn { Sin, Cos, Tan, Ln, Exp, Sqrt };

string fmtNum(double v)
{
    if (!isfinite(v)) return "nan";
    if (fabs(v - round(v)) < 1e-12 && fabs(v) < 1e15)
        return to_string((long long)llround(v));
    ostringstream os;
    os << setprecision(10) << v;
    return os.str();
}

struct Node
{
    virtual ~Node() = default;
    virtual string str(int prec) const = 0;   // prec = parent precedence
    virtual NodePtr d(const string& v) const = 0;  // partial derivative wrt v
    virtual bool has(const string& v) const = 0;   // does v occur?
    string str() const { return str(0); }
};

struct ConstN : Node
{
    double v;
    ConstN(double x) : v(x) {}
    string str(int) const override { return fmtNum(v); }
    NodePtr d(const string&) const override;
    bool has(const string&) const override { return false; }
};

struct VarN : Node
{
    string name;
    VarN(string n) : name(move(n)) {}
    string str(int) const override { return name; }
    NodePtr d(const string& v) const override;
    bool has(const string& v) const override { return name == v; }
};

struct NegN : Node
{
    NodePtr a;
    NegN(NodePtr x) : a(move(x)) {}
    string str(int prec) const override
    {
        string s = "-" + a->str(4);
        return (3 < prec) ? "(" + s + ")" : s;
    }
    NodePtr d(const string& v) const override;
    bool has(const string& v) const override { return a->has(v); }
};

struct BinN : Node
{
    Op op;
    NodePtr l, r;
    BinN(Op o, NodePtr x, NodePtr y) : op(o), l(move(x)), r(move(y)) {}

    int prec() const
    {
        switch (op)
        {
            case Op::Add: case Op::Sub: return 1;
            case Op::Mul: case Op::Div: return 2;
            default: return 4;
        }
    }

    string opS() const
    {
        switch (op)
        {
            case Op::Add: return "+";
            case Op::Sub: return "-";
            case Op::Mul: return "*";
            case Op::Div: return "/";
            default: return "^";
        }
    }

    string str(int prec) const override
    {
        int lp, rp;
        switch (op)
        {
            case Op::Add: case Op::Sub: lp = 1; rp = 2; break;
            case Op::Mul: case Op::Div: lp = 2; rp = 3; break;
            default: lp = 5; rp = 4; break;   // Pow: left needs parens, right associative
        }
        string s = l->str(lp) + opS() + r->str(rp);
        return (this->prec() < prec) ? "(" + s + ")" : s;
    }

    NodePtr d(const string& v) const override;
    bool has(const string& v) const override { return l->has(v) || r->has(v); }
};

struct FunN : Node
{
    Fn f;
    NodePtr a;
    FunN(Fn fn, NodePtr x) : f(fn), a(move(x)) {}

    string name() const
    {
        switch (f)
        {
            case Fn::Sin: return "sin";
            case Fn::Cos: return "cos";
            case Fn::Tan: return "tan";
            case Fn::Ln: return "ln";
            case Fn::Exp: return "exp";
            default: return "sqrt";
        }
    }

    string str(int) const override { return name() + "(" + a->str(0) + ")"; }
    NodePtr d(const string& v) const override;
    bool has(const string& v) const override { return a->has(v); }
};

// ---------- factory declarations ----------
NodePtr mkConst(double);
NodePtr mkVar(string);
NodePtr mkNeg(NodePtr);
NodePtr mkBin(Op, NodePtr, NodePtr);
NodePtr mkFun(Fn, NodePtr);

bool constVal(NodePtr n, double& out)
{
    auto c = dynamic_pointer_cast<ConstN>(n);
    if (c) { out = c->v; return true; }
    return false;
}

// ---------- smart constructors (basic simplification) ----------
NodePtr mkConst(double v) { return make_shared<ConstN>(v); }
NodePtr mkVar(string n) { return make_shared<VarN>(move(n)); }

NodePtr mkNeg(NodePtr a)
{
    double c;
    if (constVal(a, c)) return mkConst(-c);
    auto n = dynamic_pointer_cast<NegN>(a);
    if (n) return n->a;
    return make_shared<NegN>(a);
}

NodePtr mkBin(Op op, NodePtr a, NodePtr b)
{
    double ca = 0, cb = 0;
    bool aC = constVal(a, ca), bC = constVal(b, cb);

    if (aC && bC)
    {
        double res = 0;
        bool ok = true;
        switch (op)
        {
            case Op::Add: res = ca + cb; break;
            case Op::Sub: res = ca - cb; break;
            case Op::Mul: res = ca * cb; break;
            case Op::Div: if (cb == 0) ok = false; else res = ca / cb; break;
            default: res = pow(ca, cb); if (!isfinite(res)) ok = false; break;
        }
        if (ok && isfinite(res)) return mkConst(res);
    }

    switch (op)
    {
        case Op::Add:
            if (aC && ca == 0) return b;
            if (bC && cb == 0) return a;
            break;
        case Op::Sub:
            if (bC && cb == 0) return a;
            if (aC && ca == 0) return mkNeg(b);
            if (a->str() == b->str()) return mkConst(0);
            break;
        case Op::Mul:
            if ((aC && ca == 0) || (bC && cb == 0)) return mkConst(0);
            if (aC && ca == 1) return b;
            if (bC && cb == 1) return a;
            if (aC && ca == -1) return mkNeg(b);
            if (bC && cb == -1) return mkNeg(a);
            break;
        case Op::Div:
            if (aC && ca == 0) return mkConst(0);
            if (bC && cb == 1) return a;
            if (a->str() == b->str()) return mkConst(1);
            break;
        case Op::Pow:
            if (bC && cb == 0) return mkConst(1);
            if (bC && cb == 1) return a;
            if (aC && ca == 1) return mkConst(1);
            break;
    }
    return make_shared<BinN>(op, a, b);
}

NodePtr mkFun(Fn f, NodePtr a)
{
    double c;
    if (constVal(a, c))     // constant-fold, e.g. ln(e) -> 1
    {
        double res = 0;
        bool ok = true;
        switch (f)
        {
            case Fn::Sin: res = sin(c); break;
            case Fn::Cos: res = cos(c); break;
            case Fn::Tan: res = tan(c); break;
            case Fn::Ln: if (c > 0) res = log(c); else ok = false; break;
            case Fn::Exp: res = exp(c); break;
            default: if (c >= 0) res = sqrt(c); else ok = false; break;
        }
        if (ok && isfinite(res)) return mkConst(res);
    }
    return make_shared<FunN>(f, a);
}

// ---------- differentiation rules ----------
NodePtr ConstN::d(const string&) const { return mkConst(0); }

NodePtr VarN::d(const string& v) const { return mkConst(name == v ? 1 : 0); }

NodePtr NegN::d(const string& v) const { return mkNeg(a->d(v)); }

NodePtr BinN::d(const string& v) const
{
    switch (op)
    {
        case Op::Add:
            return mkBin(Op::Add, l->d(v), r->d(v));
        case Op::Sub:
            return mkBin(Op::Sub, l->d(v), r->d(v));
        case Op::Mul:   // product rule
            return mkBin(Op::Add,
                         mkBin(Op::Mul, l->d(v), r),
                         mkBin(Op::Mul, l, r->d(v)));
        case Op::Div:   // quotient rule
            return mkBin(Op::Div,
                         mkBin(Op::Sub,
                               mkBin(Op::Mul, l->d(v), r),
                               mkBin(Op::Mul, l, r->d(v))),
                         mkBin(Op::Pow, r, mkConst(2)));
        default:        // power / exponential / general case
        {
            if (!r->has(v))   // constant exponent: power rule  b*a^(b-1)*a'
                return mkBin(Op::Mul,
                             mkBin(Op::Mul, r, mkBin(Op::Pow, l, mkBin(Op::Sub, r, mkConst(1)))),
                             l->d(v));
            if (!l->has(v))   // constant base: a^b * ln(a) * b'
                return mkBin(Op::Mul,
                             mkBin(Op::Mul, mkBin(Op::Pow, l, r), mkFun(Fn::Ln, l)),
                             r->d(v));
            // general: a^b * (b' ln a + b a'/a)
            return mkBin(Op::Mul,
                         mkBin(Op::Pow, l, r),
                         mkBin(Op::Add,
                               mkBin(Op::Mul, r->d(v), mkFun(Fn::Ln, l)),
                               mkBin(Op::Mul, r, mkBin(Op::Div, l->d(v), l))));
        }
    }
}

NodePtr FunN::d(const string& v) const
{
    NodePtr du = a->d(v);
    switch (f)
    {
        case Fn::Sin: return mkBin(Op::Mul, mkFun(Fn::Cos, a), du);
        case Fn::Cos: return mkNeg(mkBin(Op::Mul, mkFun(Fn::Sin, a), du));
        case Fn::Tan: return mkBin(Op::Div, du, mkBin(Op::Pow, mkFun(Fn::Cos, a), mkConst(2)));
        case Fn::Ln:  return mkBin(Op::Div, du, a);
        case Fn::Exp: return mkBin(Op::Mul, mkFun(Fn::Exp, a), du);
        default:      return mkBin(Op::Div, du, mkBin(Op::Mul, mkConst(2), mkFun(Fn::Sqrt, a)));
    }
}

// ---------- tokenizer / parser ----------
struct Token
{
    enum class T { Num, Id, Op, LP, RP, End } t;
    string s;
    double v;
};

vector<Token> tokenize(const string& in)
{
    vector<Token> tk;
    size_t i = 0;
    while (i < in.size())
    {
        char c = in[i];
        if (isspace((unsigned char)c)) { ++i; continue; }

        if (isdigit((unsigned char)c) || (c == '.' && i + 1 < in.size() && isdigit((unsigned char)in[i + 1])))
        {
            size_t consumed = 0;
            double val = stod(in.substr(i), &consumed);
            tk.push_back({Token::T::Num, "", val});
            i += consumed;
            continue;
        }
        if (isalpha((unsigned char)c) || c == '_')
        {
            string s;
            while (i < in.size() && (isalnum((unsigned char)in[i]) || in[i] == '_')) s += tolower(in[i++]);
            tk.push_back({Token::T::Id, s, 0});
            continue;
        }
        if (c == '(') { tk.push_back({Token::T::LP, "(", 0}); ++i; continue; }
        if (c == ')') { tk.push_back({Token::T::RP, ")", 0}); ++i; continue; }
        if (c == '*' && i + 1 < in.size() && in[i + 1] == '*') { tk.push_back({Token::T::Op, "^", 0}); i += 2; continue; }
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') { tk.push_back({Token::T::Op, string(1, c), 0}); ++i; continue; }
        throw runtime_error(string("unexpected character: ") + c);
    }
    tk.push_back({Token::T::End, "", 0});
    return tk;
}

class Parser
{
    vector<Token> tk;
    size_t pos = 0;

    const Token& peek() const { return tk.at(pos); }
    Token take() { return tk.at(pos++); }
    bool matchOp(const string& o)
    {
        if (peek().t == Token::T::Op && peek().s == o) { ++pos; return true; }
        return false;
    }
    void expect(Token::T t, const string& msg)
    {
        if (peek().t != t) throw runtime_error(msg);
        ++pos;
    }

    NodePtr expression()
    {
        NodePtr left = term();
        for (;;)
        {
            if (matchOp("+")) left = mkBin(Op::Add, left, term());
            else if (matchOp("-")) left = mkBin(Op::Sub, left, term());
            else break;
        }
        return left;
    }

    NodePtr term()
    {
        NodePtr left = unary();
        for (;;)
        {
            if (matchOp("*")) left = mkBin(Op::Mul, left, unary());
            else if (matchOp("/")) left = mkBin(Op::Div, left, unary());
            else if (peek().t == Token::T::Num || peek().t == Token::T::Id || peek().t == Token::T::LP)
                left = mkBin(Op::Mul, left, unary());      // implicit multiplication: 2x, x y, 3sin(x)
            else break;
        }
        return left;
    }

    NodePtr unary()
    {
        if (matchOp("+")) return unary();
        if (matchOp("-")) return mkNeg(unary());
        return power();
    }

    NodePtr power()
    {
        NodePtr base = primary();
        if (matchOp("^")) return mkBin(Op::Pow, base, unary());
        return base;
    }

    NodePtr primary()
    {
        Token t = take();
        if (t.t == Token::T::Num) return mkConst(t.v);
        if (t.t == Token::T::LP)
        {
            NodePtr e = expression();
            expect(Token::T::RP, "expected ')'");
            return e;
        }
        if (t.t == Token::T::Id)
        {
            const string& n = t.s;
            Fn f;
            bool isFun = true;
            if (n == "sin") f = Fn::Sin;
            else if (n == "cos") f = Fn::Cos;
            else if (n == "tan") f = Fn::Tan;
            else if (n == "ln" || n == "log") f = Fn::Ln;
            else if (n == "exp") f = Fn::Exp;
            else if (n == "sqrt") f = Fn::Sqrt;
            else isFun = false;

            if (isFun)
            {
                expect(Token::T::LP, "expected '(' after function");
                NodePtr a = expression();
                expect(Token::T::RP, "expected ')'");
                return mkFun(f, a);
            }
            if (n == "pi") return mkConst(acos(-1.0));
            if (n == "e") return mkConst(exp(1.0));
            return mkVar(n);
        }
        throw runtime_error("unexpected token");
    }

public:
    explicit Parser(vector<Token> t) : tk(move(t)) {}
    NodePtr parse()
    {
        NodePtr e = expression();
        if (peek().t != Token::T::End) throw runtime_error("unexpected trailing input");
        return e;
    }
};

// ---------- driver ----------
int main()
{
    cout << "Partial / High-Order / Mixed Partial Derivative Engine\n";
    cout << "Syntax: + - * / ^, implicit multiplication, sin cos tan ln log exp sqrt, pi, e\n";
    cout << "Variables: any identifier, e.g. x, y, z, t\n";
    cout << "Example expression : x^2*y + sin(x*y)\n";
    cout << "Example sequence   : x, x, y   (means d/dy of d/dx of d/dx)\n\n";

    string line;
    for (;;)
    {
        cout << "f = ";
        if (!getline(cin, line)) break;
        if (line.find_first_not_of(" \t") == string::npos) break;

        cout << "differentiate w.r.t. (sequence, e.g. x, x, y): ";
        string seqLine;
        if (!getline(cin, seqLine)) break;

        try
        {
            NodePtr f = Parser(tokenize(line)).parse();
            cout << "f = " << f->str() << "\n";

            vector<string> seq;
            {
                istringstream is(seqLine);
                string tok;
                while (is >> tok)
                {
                    if (tok == ",") continue;
                    if (!tok.empty() && tok.back() == ',') tok.pop_back();
                    if (!tok.empty()) seq.push_back(tok);
                }
            }

            NodePtr cur = f;
            string acc;
            int order = 0;
            for (const string& v : seq)
            {
                cur = cur->d(v);
                ++order;
                if (!acc.empty()) acc += ",";
                acc += v;
                cout << "  d^" << order << " f / d(" << acc << ") = " << cur->str() << "\n";
            }
        }
        catch (const exception& ex)
        {
            cerr << "Error: " << ex.what() << "\n";
        }
        cout << "\n";
    }
    return 0;
}