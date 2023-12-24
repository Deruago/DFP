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
- Allow any mixing of types
- Allow custom member function invocations
- Add (runtime) optimization mechanisms

