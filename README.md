# Funtalk
A pure server-oriented programming language with sandboxed execution

## Quick Start
```bash
git clone https://github.com/funtalk-lang/funtalk.git
cd funtalk
g++ main.cpp compiler/*.cpp types/dynamic.cpp -std=c++20 -Os -UDEBUG -o funtalk && ./funtalk main.fun
```

## Test
```bash
g++ tests/flood.cpp -o test && ./test
g++ tests/slowloris.cpp -o test && ./test
g++ tests/bytes.cpp types/bytes.cpp -DFUNTALK_TEST_BYTES -std=c++20 -o test && ./test
```

## Warning
The interpreter architecture might change to a stack VM or a JIT. The core philosophy is unchangeable.

## Features
- Pure functions and messages (thanks to Haskell and Smalltalk)
- Math First: lazy-evaluation, complex numbers, matrices
- HTTP GET/POST with custom endpoints
- Deferred and async execution with thread pool and custom timeout
- Isolated code string evaluation that creates a new instance of Funtalk VM with a custom sender object and a dedicated GC
- Compatible with musl libc (thanks to Alpine Linux)

## Examples
### Comments
Use the symbol `;` for code comment
```funtalk
main() = { ; defining the main message handler
	.print(1) ; sending the print message to the object that sent main
}
```

### Online sandbox server example
```funtalk
exec(client, bytes) = {
    .send(client, ASYNC (EVAL, (STRING bytes), {
        out = ""
        print(a) = {
            a1 = ADD (STRING a) "\n"
            out = ADD (ADD (ADD out (BYTES 0)) (BYTES (SIZE a1))) a1
        }
        draw(t, color, stroke_width, title, x_label, y_label) = {
            r = STRING (BYTES 1)
            r = ADD r (BYTES color)
            r = ADD r (BYTES (FLOAT stroke_width))
            r = ADD r (BYTES (SIZE t))
            r = ADD r (BYTES (SIZE title))
            r = ADD r (BYTES title)
            r = ADD r (BYTES (SIZE x_label))
            r = ADD r (BYTES x_label)
            r = ADD r (BYTES (SIZE y_label))
            r = ADD r (BYTES y_label)
            arr = ARR t
            f i v = BYTES v
            arr.map(f)
            r = ADD r (REDUCE ADD arr "")
            out = ADD out r
        }
    }, (), (), "out") 10) ; the last number is timeout in milliseconds
}
error(e) = {
    .print(e)
}
main() = {
    port = 8080
    .handle_http(port,
        ("post", "/", "exec"),
    )
    .print(ADD "Server successfully running on port " port)
}
```
### Newton's method
```funtalk
RGBA r g b a = ADD (ADD (ADD r (SHL g 8)) (SHL b 16)) (SHL a 24)
RGB r g b = RGBA r g b 255
main() = {
    NEXT_X f df x = ADD x (MUL (NEG (f x)) (INV (df x)))
    CHECK_X eps x = LT (ABS (ADD (ADD x (MUL (NEG (f x)) (INV (df x)))) (NEG x))) eps
    f x = ADD (ADD (POW x 3) (NEG (MUL 2 x))) -2
    df x = ADD (MUL (POW x 2) 3) -2
    it = ITERATE (NEXT_X f df) 20.0
    .draw(TAKE 10 it, RGB 255 0 0, 1, "title", "X", "Y")
    it.filter(CHECK_X 0.000000000000001)
    .print(TAKE 1 it)
}
```
### Jacobi's method
```funtalk
A = MATRIX [[4.0, 1.0], [1.0, 3.0]]
b = MATRIX [[1], [2]]
DIAG m = MATRIX [
    [GET m (0,0)],
    [GET m (1,1)],
]
DIAGFLAT m = MATRIX [
    [GET m (0,0), 0],
    [0, GET m (1,0)],
]
R a d = ADD a (NEG (DIAGFLAT d))
x = b
x_new b r d x = MUL (ADD b (NEG (MULM r x))) (INV d)
LINALG a b = GET {
    t = (
        ABS (ADD (GET a (0,0)) (NEG (GET b (0,0)))),
        ABS (ADD (GET a (1,0)) (NEG (GET b (1,0)))),
    )
    res = GET t (LT (GET t 0) (GET t 1))
} "res"
main() = {
    next = x_new b (R A (DIAG A)) (DIAG A)
    it = ITERATE next x
    check x = LT (LINALG x (next x)) 0.000001
    it.filter(check)
    .print(TAKE 1 it)
}
```
## Server-Oriented Programming (SOP)
SOP is a declarative coding style that separates pure mathematical functions and pure message passing.
### Core Building Blocks
- **Object**: A container for internal state and message handlers
- **State**: A data held within an object
- **Message**: A data payload sent to trigger an object's behavior
- **Function**: A pure data mapping rule with no side effects

### The Four Main Rules
- An object can be mutated only via incoming messages.
- A message mutates only the object that receives it.
- A message can be initiated only as a consequence of processing another message.
- A message cannot return a value directly; instead, it responds by sending a new message back to the sender.

To adhere to these rules, we need to examine how the runtime works.

### The Built-In Runtime Parts
- **The Main Sender:** A built-in object that sends `main` to the Server Object and receives messages like `print` or `handle_http` to interact with the world via stdout or HTTP.
- **The Server Object Parent:** A built-in object that contains all the built-in functions; its context is used by the Server Object.
- **The Main VM:** The only Funtalk virtual machine that can interact directly with the world via the Main Sender.
- **The Main Thread:** The primary execution thread that runs the `main` entry point. Upon completing the `main` sequence, it transitions into a continuous loop that intercepts incoming HTTP traffic, manages async task timeouts, and executes deferred tasks.

To understand the runtime, we also need to look at the interpreter architecture.

### The Interpreter Architecture
- **Tokenizer:** Creates a flat AST by splitting the code text into tokens and checking for compliance with common language rules.
- **Compressor:** Reduces token types and replaces names with integer IDs.
- **Flat AST Walker:** Recursively constructs objects, calls functions, send messages.
- **GC:** Contains all values larger than 8 bytes. There are 3 types of GC:
    - **Mutable:** A value is passed by copy and cannot be used for hash tables.
    - **Immutable:** A value is passed by const reference; there cannot be 2 equal values in the pool, so their references are always equal and can be used for hash tables.
    - **Partialy Mutable:** After copying, the value gets immutable for memory optimization; after receiving a message the value gets mutable for performance optimization.

## Simplicity
It has only two operators (`=` and `.`) and two keywords (`TRUE` and `FALSE`)

## Error Handling
Two ways to handle errors
### Function Error
A function can return an `ERROR` type value
```funtalk
main() = {
    .print(1 1) ; CallError: Cannot call INT
}
```
### Message Error
An object can send the `error` message to the Server Object
```funtalk
error(e) = {
    .print(e)
}
main() = {
    a = {

    }
    a.m() ; Error in 'main': OBJ variable 'a' cannot handle message m
}
```
### Fatal Error
Some errors are not caused by a function call or a message send so they cause the program to send the error string to the client and skip the current iteration
```funtalk
main() = {
    .print(a) ; Error in 'main': Variable 'a' is not defined
    .print("Hello") ; Won't execute
}
```

## Command-Line Arguments
```bash
./funtalk main.fun 1 2
```
```funtalk
main(args) = {
	.print(args) ; (1,2,)
}
```
## SOP is inspired by these ideas:
1. Alpine Linux - Small. Simple. Secure.
2. Haskell - pure functional.
3. Smalltalk - the true OOP.
4. Calimero - il pulcino nero.
