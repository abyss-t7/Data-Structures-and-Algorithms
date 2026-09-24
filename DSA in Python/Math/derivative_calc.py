"""
 Derivative Calculator (Python / SymPy)

Solves, symbolically:
    - Ordinary high-order derivatives      d^n f / dx^n
    - Partial derivatives                  df/dx , df/dy , ...
    - Higher-order mixed partial derivs    d^3 f / dx^2 dy , etc.

SymPy does the actual calculus (parsing + differentiation rules); this
script wraps it in an interactive, step-by-step CLI so you can type in
any function and immediately see each derivative worked out.

SYNTAX FOR EXPRESSIONS
-----------------------
    Operators : +  -  *  /  **        (use * explicitly: 2*x, not 2x)
    Functions : sin(x) cos(x) tan(x) exp(x) log(x) sqrt(x)  ... (any SymPy fn)
    Constants : pi, E
    Example   : x**3*y - sin(x*y) + exp(y)/x - log(x) + sqrt(x*y)

Run:  python3 derivative_calculator.py
    (requires: pip install sympy)
    
"""

import sympy as sp
from sympy.parsing.sympy_parser import (
    parse_expr, standard_transformations, implicit_multiplication_application
)

# Allow "2x" style implicit multiplication too, as a convenience on top of
# requiring explicit '*' -- both forms will work.
TRANSFORMS = standard_transformations + (implicit_multiplication_application,)


def parse_function(text):
    """Parse a user-typed string into a SymPy expression."""
    expr = parse_expr(text, transformations=TRANSFORMS)
    return sp.sympify(expr)


def free_vars(expr):
    """Return the expression's free symbols, sorted by name for stable output."""
    return sorted(expr.free_symbols, key=lambda s: s.name)


def as_symbol(name, known_symbols):
    """Look up an existing symbol by name, or create a new one."""
    for s in known_symbols:
        if s.name == name:
            return s
    return sp.symbols(name)


def read_int(prompt):
    while True:
        raw = input(prompt).strip()
        if raw.isdigit() and int(raw) > 0:
            return int(raw)
        print("  Please enter a positive whole number.")


def read_float(prompt):
    while True:
        raw = input(prompt).strip()
        try:
            return float(raw)
        except ValueError:
            print("  Please enter a valid number.")


def print_header():
    print("========================================================")
    print(" Derivative Calculator  (ordinary, partial, mixed-partial)")
    print("========================================================")
    print("Syntax: + - * / **     functions: sin cos tan exp log sqrt ...")
    print("Example: x**3*y - sin(x*y) + exp(y)/x - log(x) + sqrt(x*y)")
    print()


def handle_high_order(f, known_syms):
    var_name = input("  Differentiate with respect to which variable? ").strip()
    var = as_symbol(var_name, known_syms)
    order = read_int("  Order n (e.g. 1, 2, 3, ...): ")

    result = f
    for k in range(1, order + 1):
        result = sp.simplify(sp.diff(result, var))
        print(f"  d^{k}f/d{var_name}^{k} = {result}")

    print(f"\n  Final answer:  d^{order}f/d{var_name}^{order} = {result}\n")
    return result


def handle_mixed_partial(f, known_syms):
    print("  Enter variables in the order you want to differentiate,")
    print("  separated by spaces (e.g. 'x y' computes d/dy( d/dx(f) ) ):")
    seq = input("  > ").strip().split()
    if not seq:
        print("  No variables entered.\n")
        return f

    result = f
    for k, name in enumerate(seq, start=1):
        var = as_symbol(name, known_syms)
        result = sp.simplify(sp.diff(result, var))
        print(f"  Step {k} (d/d{name}): {result}")

    # Build tidy notation, e.g. d^3f / dx^2 dy
    counts = {}
    for name in seq:
        counts[name] = counts.get(name, 0) + 1
    notation = f"d^{len(seq)}f / " + " ".join(
        f"d{name}" + (f"^{c}" if c > 1 else "") for name, c in counts.items()
    )
    print(f"\n  Final answer:  {notation} = {result}\n")
    return result


def handle_evaluate(f, current_result):
    which = input("  Evaluate: (a) original f   (b) last computed derivative? [a/b]: ").strip().lower()
    target = current_result if which == "b" else f
    tvars = free_vars(target)

    if not tvars:
        val = sp.N(target)
        print(f"  Result: {target}  =  {val}\n")
        return

    subs = {}
    for v in tvars:
        subs[v] = read_float(f"    Value of {v} = ")

    try:
        val = sp.N(target.subs(subs))
        print(f"  Result: {target}  =  {val}\n")
    except Exception as e:
        print(f"  [Evaluation error] {e}\n")


def handle_gradient(f, known_syms):
    """Bonus option: show all first-order partial derivatives at once."""
    tvars = free_vars(f)
    if not tvars:
        print("  f is constant -- gradient is zero.\n")
        return
    print("  Gradient (all first-order partial derivatives):")
    for v in tvars:
        d = sp.simplify(sp.diff(f, v))
        print(f"    df/d{v} = {d}")
    print()


def main():
    print_header()

    while True:
        raw = input("Enter a function f(...)  [or type 'quit' to exit]:\n> ").strip()
        if raw.lower() in ("quit", "exit"):
            break
        if not raw:
            continue

        try:
            f = sp.simplify(parse_function(raw))
        except Exception as e:
            print(f"  [Parse error] {e}\n")
            continue

        known_syms = free_vars(f)
        print(f"\n  f = {f}")
        if known_syms:
            print("  Variables found:", ", ".join(str(v) for v in known_syms))
        else:
            print("  Variables found: (none -- constant expression)")
        print()

        current_result = f
        new_function = False

        while not new_function:
            print("----------------------------------------------------------------")
            print("1) High-order derivative w.r.t. ONE variable   (d^n f / dx^n)")
            print("2) Partial / mixed derivative  (custom variable sequence)")
            print("3) Evaluate f or the last derivative at a point")
            print("4) Show gradient (all first-order partials at once)")
            print("5) Enter a new function")
            print("6) Quit")
            choice = input("Choice: ").strip()

            if choice == "1":
                current_result = handle_high_order(f, known_syms)
            elif choice == "2":
                current_result = handle_mixed_partial(f, known_syms)
            elif choice == "3":
                handle_evaluate(f, current_result)
            elif choice == "4":
                handle_gradient(f, known_syms)
            elif choice == "5":
                new_function = True
            elif choice == "6":
                return
            else:
                print("  Please choose 1-6.")

    print("Goodbye!")


if __name__ == "__main__":
    main()