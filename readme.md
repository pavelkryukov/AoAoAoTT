[![Build Status](https://travis-ci.com/pavelkryukov/AoAoAoTT.svg?branch=master)](https://travis-ci.com/pavelkryukov/AoAoAoTT)[![codecov](https://codecov.io/gh/pavelkryukov/aoaoaott/branch/master/graph/badge.svg)](https://codecov.io/gh/pavelkryukov/aoaoaott)

# AoAoAoTT

AoAoAoTT is a framework to easily switch between AoS and SoA data structures.

## Basic principles

1. Input structures should not be changed.
2. Interfaces for AoS and SoA collections must match perfectly.
3. AoS and SoA collections provide STL interfaces (iterators, begin/end, ranges etc.)

## Example

Assume you have some straightforward data structure:

```c++
struct SomeDataStructure
{
    char key[256];
    int value;
    int previous_value;
    bool valid;
};
```

Iterating an array of such structures is a pretty clear operation. The only change is the magical `obj->*(&Class::member)` operator used instead of a simple `obj.member` syntax.
```c++
int find_in_aos(const char* data)
{
    // get a reference to some array of 200 structures;
    const AoS<SomeDataStructure>& vector = get_vector();
    for (size_t i = 0; i < 200; ++i)
        if (strncmp(data, vector[i]->*(&SomeDataStructure::key), 256))
            return i;
    return -1;
}
```

However, code for structure of array looks exactly the same, wherease we are iterating the consequent block of memory now!
```c++
int find_in_soa(const char* data)
{
    // get a reference to some sturcture of 200 arrays
    const SoA<SomeDataStructure>& vector = get_vector();
    for (size_t i = 0; i < 200; ++i)
        if (strncmp(data, vector[i]->*(&SomeDataStructure::key), 256))
            return i;
    return -1;
}
```
