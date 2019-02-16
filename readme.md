# AoAoAoTT

AoAoAoTT is a framework to easily switch between AoS and SoA data structures.

## Basic principles

1. Input structures should not be changed.
2. Interfaces for AoS and SoA collections must match perfectly.
3. AoS and SoA collections provide STL interfaces (iterators, begin/end, ranges etc.)

## Example

```c++

// The structure is defined in a straightforward C/C++ manner.
struct SomeDataStructure
{
    char key[256];
    int value;
    int previous_value;
    bool valid;
};

int find_in_aos(const char* data)
{
    // get a reference to some array of 200 structures;
    const AoS<SomeDataStructure>& vector = get_vector();
    for (size_t i = 0; i < 200; ++i)
        if (strncmp(data, vector[i]->(&SomeDataStructure::key), 256)
            return i;
    return -1;
}

int find_in_soa(const char* data)
{
    // get a reference to some sturcture of 200 arrays
    const SoA<SomeDataStructure>& vector = get_vector();
    
    // The code is absolutely equivalent to the code above;
    // however, we are iterating the consequent block of memory
    // and therefore we can be faster due to spatial locality
    for (size_t i = 0; i < 200; ++i)
        if (strncmp(data, vector[i]->(&SomeDataStructure::key), 256)
            return i;
    return -1;
}
```
