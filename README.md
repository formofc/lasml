# **lasml — Lambda ASseMbler Language**

A lightweight lambda calculus interpreter with practical enhancements.

## **Features**  
- **Full lambda calculus support** — Naturally
- **Native numeric operations** — Real integer arithmetic: `+`, `-`, `*`, `/`, `%`
- **Named constants** — Convenient aliases (recursion still requires Y-combinator)
- **Minimal I/O** — Exists, but don’t expect much
- **Lazy evaluation** — Call-by-need
- **Balanced performance** — Efficient within reasonable limits
- **Syntax sugar**:
  Generalized application/abstraction:
  `λ x y.(x y x)` replaces `λ x. λ y.((x y) x)`
- **Dual evaluation modes**:
  `(f M)` for lazy • `[f M]` for strict
- **Micro-caching** — 128-byte cache for 2x speed boost

## **About**  
Created because:  
1. **To kill free time** — Because I'am bored
2. **Educational value** — Learn lambda calculus the hard(er) way  

## **Quick Start**
```bash
git clone https://github.com/formofc/lasml  
cd lasml
cc src/main.c -I includes -o lasml
./lasml --help
```

## **Documentation**
- [`Examples`](examples/)
- [`Source code`](src/)
- Lambda calculus primers 

---  

## **License**  
[**MIT**](LICENSE)  

## **Inspiration**
Forked from [`misterdown/lasm`](https://github.com/misterdown/lasm) with SOME optimizations
