[![Build Status](https://travis-ci.com/pavelkryukov/AoAoAoTT.svg?branch=master)](https://travis-ci.com/pavelkryukov/AoAoAoTT)[![codecov](https://codecov.io/gh/pavelkryukov/aoaoaott/branch/master/graph/badge.svg)](https://codecov.io/gh/pavelkryukov/aoaoaott)

# AoAoAoTT

AoAoAoTT provides [AoS and SoA](https://en.wikipedia.org/wiki/AOS_and_SOA) containers interchangeable between each other in terms of interfaces.

## Basic principles

1. Input structures should not be specially prepared.
2. Interfaces for AoS and SoA containers must match perfectly.
3. AoS and SoA containers provide STL interfaces (iterators, begin/end, ranges etc.)

## Example

Assume you have a straightforward data structure:

```c++
struct SomeDataStructure
{
    std::array<char, 256> key;
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
        if (strncmp(data, (vector[i]->*(&SomeDataStructure::key)).data(), 256) == 0)
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
        if (strncmp(data, (vector[i]->*(&SomeDataStructure::key)).data(), 256) == 0)
            return i;
    return -1;
}
```

## Supported interfaces

Both AoS and SoA mimic well-known behavior of `std::vector`:

* Construction: `AoS<Structure> storage(20), storage_init(20, Structure(42));`
* Resize: `storage.resize(30, Structure(42)`
* Assignment: `storage[index] = construct_some_structure()`
* Forward iterators (Random access iterators are in progress).

However, access to elements is performed with magic operators:
* Element access: `storage[index]->*(Structure::field)`
* Compile-time element access: `storage[index].get<Structure::field>()` 
* Object extraction: `Structure s = storage[index].aggregate_object()`

## Known limitations

### Only [trivially copyable](https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable) types are supported

That has a reason: how would you _adjust_ copy methods, move methods, or destructors if the fields are distributed all around the memory?

### C-style arrays are not supported in assignment

You cannot assign a structure with C-style array to SoA container:

```c++
   struct Example {
       char array[128];
   };
   SoA<Example> storage(1);
   storage[0] = Example(); // Does not work;
```

However, you can use `std::array` without problems:

```c++
   struct Example {
       std::array<char, 128> array;
   };
   SoA<Example> storage(1);
   storage[0] = Example(); // Correct
```
