# **lasm — lambda asm**  

A relatively lightweight lambda calculus interpreter with a few extra features.  

## **Features**  
- **Full lambda calculus support** — obviously
- **Numeric primitives** — Works with actual integers, not just Church encodings. `+`, `-`, `*`, `/`
- **Constants** — Named values for convenience. They won’t help with recursion — use the Y-combinator instead.  
- **I/O** — Exists, but don’t expect much (just being honest)  
- **Lazy evaluation** — Call-by-need  
- **Performance** — Decent, but don’t push it  
- **Application sugar**:  
  Classic λ-calculus requires exactly 2 arguments per application, but here it’s generalized for brevity:  
  `((x x) x)` → `(x x x)`  
- **Both lazy and strict applications**: lazy via `(f M)`, strict via `[]`

## **About**  
Created because:  
1. **First reason** — To kill some free time  
2. **Second reason** — Educational purposes  

## **Quick Start**  
```bash  
git clone https://github.com/misterdown/lasm  
cd lasm  
cc src/main.c -I includes -o lasm
lasm --help
```  

## **Documentation**  
Need more details? Check out:  
- [Examples](examples/)  
- The source code  
- Any good λ-calculus article  

---  

## **License**  
[**MIT**](LICENSE)  
