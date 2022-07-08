# Katara

Katara is a custom programming language inspired by Go. I am developing the compiler from scratch 

## Overview

The compiler works in several stages, illustrated for the example program below:

```
package main

func main() int {
    var sum int
    for i := 0; i < 10; i++ {
        sum += i
    }
    return sum
}
```

1. The [scanner](https://github.com/arneph/Katara/tree/master/src/lang/processors/scanner) and 
[parser](https://github.com/arneph/Katara/tree/master/src/lang/processors/parser) generate an 
[AST](https://github.com/arneph/Katara/tree/master/src/lang/representation/ast):

![AST for example program](https://github.com/arneph/Katara/blob/master/docs/example1.ast.png?raw=true)

2. The [type checker](https://github.com/arneph/Katara/tree/master/src/lang/processors/type_checker)
resolves all identifiers, such as function and variable names, builtin type 
names, etc. (see 
[identifier resolver](https://github.com/arneph/Katara/blob/master/src/lang/processors/type_checker/identifier_resolver.cc)).

3. The type checker finds all entities (identifiers, expressions, functions, et.c) that need 
[type information](https://github.com/arneph/Katara/tree/master/src/lang/representation/types)
associated with them and their depedencies for type checking. For example: a function 
might have an argument with a type alias defined elsewhere. The type checker then adds type 
information for everything in order. The result looks something like:

```
Types:
test2.kat:6:2  sum    int           
test2.kat:5:21 i      int           
test2.kat:5:13 i < 10 bool (untyped)
test2.kat:3:12 int    int           
test2.kat:5:13 i      int           
test2.kat:6:9  i      int           
test2.kat:5:17 10     int (untyped) 
test2.kat:8:8  sum    int           
test2.kat:4:9  int    int           
test2.kat:5:10 0      int (untyped) 

Constant Expressions:
test2.kat:5:17 10 10
test2.kat:5:10 0  0 

Constants:

Definitions:
test2.kat:5:5 i    var i int      
test2.kat:4:5 sum  var sum int    
test2.kat:3:5 main func main() int

Uses:
test2.kat:6:2  sum var sum int
test2.kat:6:9  i   var i int  
test2.kat:3:12 int type int   
test2.kat:5:13 i   var i int  
test2.kat:8:8  sum var sum int
test2.kat:5:5  i   var i int  
test2.kat:4:9  int type int   
test2.kat:5:21 i   var i int  

Implicits:
test2.kat:3:12 var  int
```

At this point, the AST and type information can be used to generate HTML versions of the source 
files with syntax highlighting and highlights for identifers that refer to the same entity. This is
available with the `$ katara doc` command.

![Screenshot of HTML for example program](https://github.com/arneph/Katara/blob/master/docs/example1.html.png?raw=true)

If there are any [issues](https://github.com/arneph/Katara/blob/master/src/lang/processors/issues/issues.h)
with the code, they will be found and printed during one of the first three phases.

4. The [IR builder](https://github.com/arneph/Katara/tree/master/src/lang/processors/ir/builder) 
generates a 
[high level intermediate representation](https://github.com/arneph/Katara/tree/master/src/lang/representation/ir_extension) 
in [SSA-form](https://en.wikipedia.org/wiki/Static_single-assignment_form) from the AST and type 
information:

```
@0 main () => (i64) {
{0}
    %0:lshared_ptr<i64, s> = make_shared
    store %0, #0:i64
    %1:lshared_ptr<i64, s> = make_shared
    store %1, #0:i64
    jmp {1}
{1}
    %2:i64 = load %1
    %3:b = ilss %2, #10:i64
    jcc %3, {4}, {3}
{2}
    %4:i64 = load %1
    %5:i64 = iadd %4, #1:i64
    store %1, %5
    jmp {1}
{3}
    delete_shared %1
    %9:i64 = load %0
    delete_shared %0
    ret %9
{4}
    %6:i64 = load %1
    %7:i64 = load %0
    %8:i64 = iadd %7, %6
    store %0, %8
    jmp {2}
}
```

Initially, all local variables are treated as reference counted (shared) heap allocations. This is 
makes it possible to capture them in inner functions that outlive the stack frame or return the 
address of a local variable from a function.

Katara generates a control flow graph and dominator tree for this IR:

![Control flow graph](https://github.com/arneph/Katara/blob/master/docs/example1.cfg.dot.png?raw=true)
![Dominator tree](https://github.com/arneph/Katara/blob/master/docs/example1.dom.dot.png?raw=true)

5. Optimizers for the high level IR attempt to convert 
[from shared to unique (not reference counted) pointers](https://github.com/arneph/Katara/blob/master/src/lang/processors/ir/optimizers/shared_to_unique_pointer_optimizer.cc)
and [from unique pointers to local values](https://github.com/arneph/Katara/blob/master/src/lang/processors/ir/optimizers/unique_pointer_to_local_value_optimizer.cc).

After the first optimization, the IR looks like this:

```
@0 main() => (i64) {
{0}
  %0:lunique_ptr<i64> = make_unique #1:i64
  store %0, #0:i64
  %1:lunique_ptr<i64> = make_unique #1:i64
  store %1, #0:i64
  jmp {1}
{1}
  %2:i64 = load %1
  %3:b = ilss %2, #10:i64
  jcc %3, {4}, {3}
{2}
  %4:i64 = load %1
  %5:i64 = iadd %4, #1:i64
  store %1, %5
  jmp {1}
{3}
  delete_unique %1
  %9:i64 = load %0
  delete_unique %0
  ret %9
{4}
  %6:i64 = load %1
  %7:i64 = load %0
  %8:i64 = iadd %7, %6
  store %0, %8
  jmp {2}
}
```

After the second optimization, the IR changes to:

```
@0 main () => (i64) {
{0}
    jmp {1}
{1}
    %2 = phi %5{2}, #0{0}
    %9 = phi %8{2}, #0{0}
    %3:b = ilss %2, #10:i64
    jcc %3, {4}, {3}
{2}
    %4:i64 = mov %2
    %5:i64 = iadd %4, #1:i64
    jmp {1}
{3}
    ret %9
{4}
    %6:i64 = mov %2
    %7:i64 = mov %9
    %8:i64 = iadd %7, %6
    jmp {2}
}
```

6. High level constructs, such as shared and unique pointers, strings, and panic instructions, get
lowered to more 
[basic IR instructions](https://github.com/arneph/Katara/blob/master/src/ir/representation/instrs.h)
and function calls to the language runtime.

7. Using [IR anlayers](https://github.com/arneph/Katara/tree/master/src/ir/analyzers), the low level IR gets further optimized, removing 
[unused functions](https://github.com/arneph/Katara/blob/master/src/ir/optimizers/func_call_graph_optimizer.cc) 
and code. In the future, 
constant folding, common subexpression elimination, function inlining, etc. will be implemented 
during this phase.

At this point the IR can be 
[interpreted](https://github.com/arneph/Katara/blob/master/src/ir/interpreter/interpreter.h) 
with the `$ katara interpret` command, instead of further compiling it.

8. All phi instructions get
[resolved](https://github.com/arneph/Katara/blob/master/src/ir/processors/phi_resolver.cc)
to mov instructions in the respective parent blocks of blocks 
with phi instructions.

9. The [life ranges](https://github.com/arneph/Katara/blob/master/src/ir/info/func_live_ranges.h)
for all SSA values to be stored in registers get 
[determined](https://github.com/arneph/Katara/blob/master/src/ir/analyzers/live_range_analyzer.cc):

```
live ranges for @0 main:
  {4} - live ranges:
<----> %9
 +-+   %6
<----> %2
  ++   %7
   +-> %8
entry set: %2, %9
 exit set: %8, %2, %9

  {3} - live ranges:
<+  %9
entry set: %9
 exit set: 

  {2} - live ranges:
<---> %8
<---> %2
<---> %9
 ++   %4
  +-> %5
entry set: %9, %2, %8
 exit set: %5, %9, %2, %8

  {1} - live ranges:
<----> %9
<----> %2
   ++  %3
entry set: %2, %9
 exit set: %2, %9

  {0} - live ranges:
<-> %9
<-> %2
entry set: %2, %9
 exit set: %2, %9
```

10. An 
[interference graph](https://github.com/arneph/Katara/blob/master/src/ir/info/interference_graph.h) 
between all register values gets 
[built](https://github.com/arneph/Katara/blob/master/src/ir/analyzers/interference_graph_builder.cc)
and 
[colored](https://github.com/arneph/Katara/blob/master/src/ir/analyzers/interference_graph_colorer.cc):

![Colored interference graph for example program](https://github.com/arneph/Katara/blob/master/docs/example1.interference_graph.dot.png?raw=true)

11. Using the register assignments and live ranges, the IR gets 
[translated](https://github.com/arneph/Katara/tree/master/src/x86_64/ir_translator) 
to [x86_64](https://github.com/arneph/Katara/tree/master/src/x86_64):

```
main: ; <2>
BB0:
    push rbp
    mov rbp,rsp
    push rbx
    mov rbx,0x00000000
    mov rdx,0x00000000
    jmp BB1
BB1:
    cmp rbx,0x0000000a
    setl al
    test al,0xffffffff
    je BB3
    jmp BB4
BB2:
    mov rax,rbx
    add rax,0x00000001
    mov rbx,rax
    mov rdx,rcx
    jmp BB1
BB3:
    mov rax,rdx
    pop rbx
    pop rbp
    ret
BB4:
    mov rax,rbx
    mov rcx,rdx
    add rcx,rax
    jmp BB2
```

12. With the [`$ katara run`](https://github.com/arneph/Katara/blob/master/src/cmd/run.cc) command,
the code gets written to a new page in memory, functions (including malloc and free) get linked,
the permissions for the page get changed from write to execute, the compiled program runs in the
same process and returns with the expected exit code 45 (the sum from 0 to 9).
