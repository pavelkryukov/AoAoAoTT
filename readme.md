[![codecov](https://codecov.io/gh/pavelkryukov/aoaoaott/branch/master/graph/badge.svg)](https://codecov.io/gh/pavelkryukov/aoaoaott)

# AoAoAoTT

AoAoAoTT provides AoS and SoA containers interchangeable between each other in terms of interfaces.

More details are discussed in this talk (in Russian): [Interchangeable AoS and SoA containers, _C++ Russia 2021_](https://cppconf.ru/en/talks/interchangeable-aos-and-soa-containers/).

## Motivation

[Array of Structures and Structure of Arrays](https://en.wikipedia.org/wiki/AOS_and_SOA) are two ways to arrange a sequence of records in computer memory.
Whereas SoA is more friendly with SIMD instructions and data prefetching, AoS usually utilizes spatial and temporal locality of CPU caches.
Therefore, choose of the most performing data representation can hardly be theoretically proven, and the meainingful results may be obtained only by a quantative measurement.

However, arrangement of the experiment is laborious, as most of the programming languages natively support only AoS structures, and SoA versions of algorithms has to be made from scratch.

AoAoAoTT simplifies the process for C++.
The basic idea is to keep the data structures as they were defined by programmer, and touch only containers and access to them.

### Example

Consider the following simple example of AoS:

```c++
struct SomeDataStructure {
    std::array<char, 256> key;
    int value;
    int previous_value;
};

std::array<SomeDataStructure, 10000> storage;

int find_value(int value_to_find) {
    for (const auto& e : storage)
        if (e.value == value_to_find)
            return i;
    return -1;
}
```

Now you want to check whether SoA performs better.
With AoAoAoTT, you have to do two simple steps: replace `std::array` by `aoaoaott::Array` and access members with the magical `->*` operator instead of common `.`.

```diff
+#include <aoaoaott.hpp>
+using namespace aoaoaott;

-std::array<SomeDataStructure, 10000> storage;
+SoAArray<SomeDataStructure, 10000> storage;

 int find_value(int value_to_find) {
     for (const auto& e : storage)
-        if (e.value == value_to_find)
+        if (e->*(&SomeDataStructure::value) == value_to_find)
            return i;
    return -1;
}
```

Imagine that for some reason SoA did not perform well and you want to rollback to AoS to make more accurate measurements.
That would be very simple: just substitute `SoA` container by fully interface-compatible `AoS`.

```diff
-SoAArray<SomeDataStructure, 10000> storage;
+AoSArray<SomeDataStructure, 10000> storage;
```

With some macro or SFINAE helpers you would be able to change the arrangement easily, e.g. from the command line.

### Summary

To sum up, let's enumerate **the basic principles** of AoAoAoTT design:
1. Input structures should not be specially prepared.
2. Interfaces for AoS and SoA containers are perfectly matched.
3. AoS and SoA containers provide STL-like interfaces (iterators, begin/end, ranges etc.)
4. Minimal dependencies: header-only Boost and C++17 STL.

----
## Supported interfaces

Four containers are provided along with element facade objects and iterators: `SoAVector`, `AoSVector`, `SoAArray`, `SoAVector`;
Both AoS and SoA container mimic well-known behavior of `std::vector` and `std::array`:

* **Construction and fill:** `AoS<Structure> storage(20), storage_init(20, Structure(42));`
* **Assignment LHS:** `storage[index] = construct_some_structure()`
* **Assignment RHS:** `some_structure = storage[index]`
* Full support of **random access iterators**

Vector specific operations:
* **Resize:** `storage.resize(30, Structure(42))`
* **Push back:** `storage.push_back`
* Capacity, reserve, and shrink-to-fit.

However, access to elements is performed with magic operators:
* **Constexpr element access:** `storage[index].get<&Structure::field>()`
* **Elegant element access:** `storage[index]->*(&Structure::field)`
* **Aggregate and call a method:** `storage[index].method<&Structure::update>(param1, param2)`
* **Elegant lambda call:** `(storage[index]->*(&Structure::update))(param1, param2)`

The best and the most actual reference is provided by [unit tests](https://github.com/pavelkryukov/AoAoAoTT/blob/master/test/test.cpp).

----
## Known limitations

### Consider using Rule of Zero structures

That has a reason: how would you _adjust_ copy methods, move methods, or destructors if the fields are distributed all around the memory?
For example:

```c++
   struct Point1 {
       int x, y;
       int *z;
       ~Point1() { delete z; }
   };
```

One you move the object to SoA, C++ will consider it dead and call the destructor. Data is lost!

However, we cannot forbid anything which is not `is_trivially_...`. Consider an example of [Rule of Zero](https://cpppatterns.com/patterns/rule-of-zero.html) structure:

```c++
   struct Point2 {
       int x, y;
       std::unique_ptr<int> z;
   };
```

While this class has a non-trivial destructor, it has no issues with SoA, because `unique_ptr` will be moved correctly.

### Aggregation is costly

Similarly to the one above, you cannot run `storage[i].some_method`.
Instead, AoAoAoTT provides an interface to aggregate a structure to stack, call a method, and dissipate it back.
Obviously, these operations have overheads.

* **Q:** Can dissipation be explicitly bypassed if a method is `const`-qualified?
* **A:** No. Structure may contain `mutable` fields, and we must update them as well.

### C-style arrays are not supported

You cannot assign a structure with C-style array to SoA container:

```c++
   struct Example {
       char array[128];
   };
   SoA<Example> storage(1, Example()); // Does not work;
```

However, you can use `std::array` without any problems:

```diff
-       char array[128];
+       std::array<char, 128> array;
```

### Inherited structures are not supported

To decompose data structure, we use PFR mechanism which does not support inherited structures at the moment. The issue is described in [PFR docs](https://www.boost.org/doc/libs/1_77_0/doc/html/boost_pfr/limitations_and_configuration.html).

### Padding bytes are not supported

Since C++ reflection capabilities are very low, support of padding bytes cannot be provided at the moment.
However, if people care about SoA data representation, one might consider they have already handled padding bytes wisely.

One more obvious case is empty structures: they have a single padding byte, and that's why they could not be stored to AoAoAoTT storages.

### Packed structures are not supported

It is a undocumented restriction of PFR.

### Booleans are not supported in SoAVector

As you may guess, it is a result of STL-incompliance of `std::vector<bool>`.

----

## Impact

* [C++Russia 2021 talk](https://cppconf.ru/en/talks/interchangeable-aos-and-soa-containers/).
* LLVM bug reported: [llvm-project/llvm#52617](https://github.com/llvm/llvm-project/issues/52617).
* Few ideas for C++ standard: [cpp-ru/ideas#482](https://github.com/cpp-ru/ideas/issues/482), [cpp-ru/ideas#484](https://github.com/cpp-ru/ideas/issues/484).

## Further reading

* _**[Nomad Game Engine: Part 4.3 — AoS vs SoA](https://medium.com/@savas/nomad-game-engine-part-4-3-aos-vs-soa-storage-5bec879aa38c)** by Niko Savas_ — nice demonstration of SoA advantages.
* _**[Example of AoS outperforming SoA](https://stackoverflow.com/questions/17924705/structure-of-arrays-vs-array-of-structures-in-cuda/17924782#17924782)** by Paul R_
* _**[The C++ Type Loophole (C++14)](http://alexpolt.github.io/type-loophole.html)** by Alexandr Poltavsky_ — explanation of Loophole compile-time reflection, the core code of AoAoAoTT.

## Thanks

* [Paolo Crosetto](https://github.com/crosetto) for his inspiring [SoAvsAoS implementation](https://github.com/crosetto/SoAvsAoS)
* [Alexandr Poltavsky](https://github.com/alexpolt) for public domain [implementation of Loophole and Type List](https://github.com/alexpolt/luple)
* [Antony Polukhin](https://github.com/apolukhin) and contributors for [Boost.PRF](https://github.com/boostorg/pfr)
* [Timur Safin](https://github.com/tsafin) and [Alexey Klimkin](https://github.com/klimkin) for reviewing [C++ Russia 2021 talk](https://cppconf.ru/en/talks/interchangeable-aos-and-soa-containers/).
* [@Fuyutsubaki](https://github.com/Fuyutsubaki) for elegant pointer-to-member to index converter
* [Alexandr Titov](https://github.com/alexander-titov) for assistance with benchmarking
