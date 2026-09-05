# Radix

Radix is a minimalist, interpreted programming language and runtime system written in ANSI C with a complete, self-hosted metacircular interpreter (`radix.rx`). It features a lexical tokenizer, recursive descent parser, Abstract Syntax Tree (AST) evaluator, lexical environment model with first class closures, and an integrated mark and sweep garbage collector.

Radix possesses complete semantic reflexivity: the language can tokenize, parse, inspect, and evaluate its own grammar at arbitrary recursive interpreter nesting depths. Radix expresses any computable function and is proven Turing complete through multiple independent models of computation, including register machines, unbounded tape Turing machines, Markov rewrite systems, and metacircular evaluation.


## Features

- **Lexical Tokenizer**: Robust lexical scanner supporting numeric literals, boolean constants, string literals with escape sequences, identifiers, keywords, arithmetic, relational, and logical operators.
- **Recursive Descent Parser**: Predictive grammar parser constructing a typed Abstract Syntax Tree (AST).
- **Lexical Environment Model**: Lexical scoping with lexical closures, variable declarations (`var`), constants (`const`), and variable assignment (`=`).
- **Mark-and-Sweep Garbage Collector**: Tracing garbage collector in the C host tracking live root references across the execution call stack and evaluation value stack, enabling unbounded execution and deep self-hosting recursion without memory exhaustion.
- **Semantic Reflexivity & String Primitives**: First-class string operations (`len`, `char_at`, `substr`, `ord`, `chr`, `str`, `num`, `find`, and formatted stream output via `out` and `outn`).
- **Self-Hosted Metacircular Interpreter (`radix.rx`)**: Complete interpreter written purely in Radix, capable of executing Radix source code and interpreting further instances of itself at arbitrary depth:
  `./radix radix.rx radix.rx ... script.rx`
- **Formal Computability Proof Suite**: Verified theoretical constructions demonstrating Kleene's Second Recursion Theorem, Chomsky hierarchy Type 1 context-sensitive language recognition, unbounded universal Turing machines, and Gödel diagonalization.


## Language Overview

Radix expressions and statements follow an explicit, clean syntax:

- **Variables and Constants**: Declared via `var x = expr;` and `const c = expr;`.
- **Data Types**:
  - Numbers: Signed integers and floating-point numeric representations.
  - Booleans: `true` and `false`.
  - Strings: Double-quoted string literals with full escape sequences (`\n`, `\t`, `\r`, `\\`, `\"`).
  - Functions: First-class closures declared via `func(params) => expression` or `func(params) => { statements; return_expr; }`.
- **Operators**:
  - Arithmetic: `+`, `-`, `*`, `/`, `%`.
  - Relational: `<`, `<=`, `>`, `>=`, `==`, `!=` (supported for both numbers and lexicographical strings).
  - Logical: `&&`, `||`, `!`.
  - String concatenation: `+` automatically converts numbers and booleans when concatenated with strings.
- **Control Structures**: `if (cond) { ... } elif (cond) { ... } else { ... }` and `while (cond) { ... }`.
- **Built-in Functions**:
  - `out(val)`: Emits string or value representation to standard output without trailing newline.
  - `outn(val)`: Emits string or value representation to standard output followed by newline.
  - `read_file(path)`: Loads entire file contents into a string.
  - `len(str)`: Returns character count of string.
  - `char_at(str, index)`: Extracts single-character string at zero-based index.
  - `substr(str, start, length)`: Extracts substring slice from index `start` with given length.
  - `ord(str, [index])`: Returns ASCII integer code at specified index (defaults to index 0).
  - `chr(code)`: Constructs single-character string from ASCII integer code.
  - `str(val)`: Converts number or boolean to string representation.
  - `num(str)`: Parses string representation into numeric value.
  - `find(haystack, needle)`: Returns zero-based offset of substring, or `-1` if absent.
  - `argv(n)`: Retrieves command-line argument vector element at index `n`.
  - `argc()`: Retrieves total argument count.


## System Architecture

Radix is partitioned into a native C virtual machine (`radix.c`) and a pure Radix self-hosted metacircular interpreter (`radix.rx`).

### 1. Tokenizer and Scanner

The tokenizer scans raw ASCII source code into a linear sequence of typed tokens. It recognizes single-character symbols, multi-character relational operators (`<=`, `>=`, `==`, `!=`, `&&`, `||`), identifiers, keywords, string literals with escape sequence translation, and numerical constants. Comments starting with `#` or `//` extending to end-of-line are ignored.

### 2. Recursive Descent Parser

The parser implements recursive descent with operator precedence matching standard algebraic hierarchy. The grammar parses top-level declarations, statement blocks, looping constructs, conditionals, anonymous function abstractions, and binary expressions into a hierarchical Abstract Syntax Tree.

### 3. Lexical Scoping and First-Class Closures

Functions in Radix capture their enclosing lexical environment upon definition. The runtime allocates an `Env` structure binding variable names to value slots, maintaining a pointer to the parent environment frame. When a closure is called, a new child environment is created pointing to the captured environment of the closure, ensuring static lexical scope.

### 4. Mark-and-Sweep Garbage Collector

To support long-running recursive algorithms and multi-level metacircular self-hosting, the C host implementation integrates an exact mark-and-sweep garbage collector:

- **Root Set Tracking**: The collector traces all active execution frames via an explicit call stack (`g_call_stack`) and all temporary evaluated values through an evaluation value stack (`g_val_stack`).
- **Graph Traversal**: During the mark phase, reachable environments and closure structures are traversed recursively. Unreachable allocated frames resulting from closed scopes or finished metacircular evaluation steps are marked dead.
- **Sweep Phase**: All unmarked environment nodes and closure records are returned to the memory manager.
- **Fast-Path Symbol Resolution**: Variable lookup caches identifier pointer identity for static bindings, minimizing traversal overhead during inner loops.


## Theoretical Foundations and Formal Proofs

The Radix repository includes formal test programs located in `scripts/` validating foundational theorems in theoretical computer science.

### Turing Completeness via Counter Machine (`scripts/turing.rx`)

Simulates a universal register machine computing $5! = 120$ via nested recursive functional state transitions, demonstrating Turing equivalence in the integer domain.

### Unbounded Universal Turing Machine on String Tape (`scripts/turing_string_tape.rx`)

Implements a universal Turing machine navigating an unbounded, bidirectional tape represented as a dynamic string. Transitions are encoded as structured triples `"next_state,write_symbol,direction"` parsed at runtime. The machine increments arbitrary-length binary counters ($1$, $10$, $11$, $1100$, $100000000000000000000$), verifying unbounded memory navigation.

### Kleene's Second Recursion Theorem and the Quine (`scripts/kleene_quine.rx`)

A zero-input self-replicating program demonstrating Stephen Kleene's Second Recursion Theorem (1938): for every computable transformation $F$, there exists an index $e$ such that $\phi_{F(e)} = \phi_e$. In the identity case ($F = \text{id}$), the program outputs its own code:

```c
var q = chr(34);var s = "var q = chr(34);var s = ;out(substr(s, 0, 24) + q + s + q + substr(s, 24, len(s) - 24));";out(substr(s, 0, 24) + q + s + q + substr(s, 24, len(s) - 24));
```

The program partitions its structure into an active data template and an evaluation mechanism that reconstructs surrounding source text via `substr` and string concatenation without escaping hazards.

### Markov Algorithm Equivalence (`scripts/markov_algorithm.rx`)

Implements Andrey Markov's normal algorithms (1951), an alternative universal model of computation based on ordered string rewrite production rules. The script computes binary increments using purely symbolic substitution rules:

$$1 \to 0, \quad 0 \to 1, \quad \ast 0 \to 1, \quad \ast 1 \to 10$$

Proving computational equivalence between tape-based automata and string rewrite production systems.

### Chomsky Hierarchy: Context-Sensitive Language Recognition (`scripts/chomsky_csl.rx`)

Recognizes the canonical Context-Sensitive Language (Type 1 in the Chomsky hierarchy):

$$L = \{ a^n b^n c^n \mid n \ge 1 \}$$

By the Pumping Lemma for Regular Languages, $L$ cannot be recognized by Finite State Automata. By the Pumping Lemma for Context-Free Languages (Bar-Hillel, Perles, Shamir), $L$ cannot be recognized by Pushdown Automata. The Radix recognizer parses terminal sequences in linear time, outputting `1` for valid structures (`"abc"`, `"aabbcc"`, `"aaabbbccc"`, `"aaaaabbbbbccccc"`) and `0` for invalid perturbations (`"aabbc"`, `"baabbcc"`, `"abcabc"`, `""`).

### Gödel Sentence and Diagonalization (`scripts/godel_string.rx`, `scripts/pure_diag.rx`)

Constructs a self-referential statement within string semantics following Kurt Gödel's 1931 diagonalization method. The substitution function `diag(s)` replaces placeholder markers with quoted representations of the input text, generating propositions of the form:

$$\text{UNPROVABLE}(\text{diag}(x))$$

Evaluating the fixed-point predicate produces a string that directly asserts its own unprovability under formal derivation rules.

### Metacircular S-Expression Evaluator (`scripts/lisp_eval.rx`)

Implements a recursive-descent prefix parser and tree evaluator for symbolic S-expressions. Evaluates nested Polish prefix arithmetic (`+`, `-`, `*`, `/`, `%`), relational predicates (`<`, `>`, `=`), and conditional branching (`if`). compound state tuples $(value, position)$ are packed into string structures and evaluated dynamically.


## Self-Hosting Metacircular Interpreter (`radix.rx`)

The file `radix.rx` is a complete, self-contained interpreter for Radix written in Radix itself.

### Architecture of `radix.rx`

1. **Self-Tokenizer**: Utilizes integer ASCII scanning (`ord(src, i)`) to convert source strings into linked lists of token pairs without per-character allocations.
2. **Grammar Parser**: Full recursive-descent parser producing closures representing AST nodes (`mk_lit`, `mk_ident`, `mk_var`, `mk_assign`, `mk_bin`, `mk_if`, `mk_while`, `mk_fn`, `mk_call`, `mk_block`).
3. **Closure Environment**: Maintains variable bindings and constants using dynamic lookup cells chained to parent scopes.
4. **Direct Built-in Resolution**: Dispatches built-in operations through an optimized table, preserving small and efficient user environment frames.
5. **Argument Shifting Protocol**: Forwards command-line arguments to child interpreter layers:
   ```c
   var c_argv = argv;
   var c_argc = argc;
   env["argv"] = func(n) => c_argv(n + 1);
   env["argc"] = func() => c_argc() - 1;
   ```
   Each interpreter level consumes its target file argument and shifts the remaining vector to its child. This allows arbitrary depth nesting without hardcoded layer limits:

```
Level 0: Native Binary (./radix)
    executes
Level 1: radix.rx (Interpreted by C)
    parses and evaluates
Level 2: radix.rx (Interpreted by Level 1 radix.rx)
     parses and evaluates
Level 3: script.rx (Interpreted by Level 2 radix.rx)
```


## Building and Verification

### Compilation

Radix requires an ANSI C compiler (`gcc`, `clang`, `tcc`, or `musl-gcc`) and `make`:

```bash
make
```

This compiles the static, optimized binary `radix`.

### Comprehensive Verification Suite (`verify.sh`)

Executes 28 automated test suites verifying language syntax, arithmetic, closures, deep recursion, string operations, quine self-replication, Turing machine simulations, and Chomsky grammar recognition:

```bash
./verify.sh
```

### Self-Hosting Proof Suite (`selfhost.sh`)

Executes the metacircular self-hosting proof suite, verifying that:
1. `radix.rx` executes test programs under the host interpreter (Level 1).
2. `radix.rx` executes `radix.rx` which executes test programs (Level 2).
3. The multi-level interpreter operates at arbitrary depth (Level 3).
4. All outputs match native execution byte-for-byte.

```bash
./selfhost.sh
```

## License

Copyright (C) 2025-2026 Ivan Gaydardzhiev.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3 of the License. See [COPYING](./COPYING) for complete details.
