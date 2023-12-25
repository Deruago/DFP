# DFP: Fixpoint

DFP is a library which allows the construction of fixpoint computations without requiring any overhead.

# Usage

```C++
#include <DFP.h>
#include <iostream>

int main()
{
    double Ci = 1120;
    double Ck = 0.443;
    double Tk = 0.977;

    Fixpoint R = Ci + std::ceil(Ci / Tk) * Ck;

    auto fixpoint = (R = Ci + FixpointSpecialCeil(R / Tk) * Ck);

    // Prints: 2049.41
    std::cout << fixpoint() << '\n';

    return 0;
}
```

The example provided above computes a fixpoint equation named 'fixpoint'. The fixpoint parameter that is iterated upon is named 'R' and is set to a default value. The fixpoint equation is only evaluated when it is invoked see ```fixpoint()```.

# Future extensions

- Add more operators (currently only +, -, /, * are supported)
- Add more special functions
- Allow conditional logic
- Allow any mixing of types
- Allow custom member function invocations
- Add (runtime) optimization mechanisms

# Recursive functions
## (Recursive) Fibonacci

```C++
#include <DFP.h>
#include <iostream>

int main()
{
    Fixpoint<int> fib = 0;
    FixpointParameter n;
    fib(0) = 1;
    fib(1) = 1;
    auto fibonacci = (fib(n) = fib(n - 1) + fib(n - 2));

    // Prints: 55
    std::cout << fibonacci(9) << '\n';

    return 0;
}
```

## Factorial

```C++
#include <DFP.h>
#include <iostream>

int main()
{
    Fixpoint<int> fac = 0;
    FixpointParameter n;
    fac(0) = 1;
    fac(1) = 1;
    auto factorial = (fac(n) = fac(n - 1) * n);

    // Prints: 120
    std::cout << factorial(5) << '\n';

    return 0;
}
```
