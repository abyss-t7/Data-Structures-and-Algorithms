"""Partial, high-order, and mixed partial derivative solver using SymPy."""
import re
import sympy as sp


def prepare(text: str) -> str:
    """Accept calculator-style input: ^ for power, ln for natural log."""
    text = text.replace('^', '**')
    text = re.sub(r'\bln\(', 'log(', text)
    return text


def split_sequence(text: str):
    """'x, x, y' or 'x x y' -> ['x', 'x', 'y']"""
    return [t for t in re.split(r'[,\s]+', text.strip()) if t]


def main():
    print("Partial / High-Order / Mixed Partial Derivative Solver (SymPy)")
    print("Example expression : x^2*y + sin(x*y)")
    print("Example sequence   : x, x, y\n")

    while True:
        line = input("f = ").strip()
        if not line:
            break
        seq_line = input("differentiate w.r.t. (sequence, e.g. x, x, y): ").strip()

        try:
            f = sp.sympify(prepare(line))
            print("f =", sp.simplify(f))

            cur = f
            acc = []
            for order, name in enumerate(split_sequence(seq_line), start=1):
                var = sp.Symbol(name)
                cur = sp.diff(cur, var)          # one partial step
                acc.append(name)
                print(f"  d^{order} f / d({','.join(acc)}) =", sp.simplify(cur))

            # Direct high-order form, e.g. diff(f, x, 2, y) == the loop above:
            specs = []
            for name in split_sequence(seq_line):
                specs.append(sp.Symbol(name))
            if specs:
                print("  cross-check (single call):", sp.simplify(sp.diff(f, *specs)))
        except Exception as ex:
            print("Error:", ex)
        print()


if __name__ == "__main__":
    main()