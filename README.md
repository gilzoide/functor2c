# functor2c
Single header templates for wrapping C++ functors as opaque userdata plus function pointers for C interop.


## Features
- Easily wrap functors such as `std::function` or lambdas as function pointers to use in C APIs
- Supports functors with parameters and return values of any type
- Provides deleter functionality to avoid memory leaks, including overloads that return smart pointers
- Requires C++11 or newer
- Automatic arguments / return type deduction when used in C++17


## Example usage
```cpp
#include "functor2c.hpp"

// Let's use as an example Lua's allocator function `lua_Alloc`
// Note that it accepts an opaque userdata as the first parameter (prefix)
typedef void *(*lua_Alloc)(void *ud, void *ptr, size_t osize, size_t nsize);

// 1. Define the functor for it
auto alloc_func = [](void *ptr, size_t osize, size_t nsize) {
    ptr = /* implementation ... */;
    return ptr;
};

// 2. Now create the opaque userdata + function pointers for it
// 2.a) C++17 supports type deduction and structured bindings
auto [userdata, invoke_fptr, delete_fptr] = functor2c::prefix_invoker_deleter(alloc_func);
// 2.b) C++11 requires specifying functor type as <ReturnType, ArgumentTypes...>
void *userdata;
lua_Alloc invoke_fptr;
void (*delete_fptr)(void*);
std::tie(userdata, invoke_fptr, delete_fptr) = functor2c::prefix_invoker_deleter<void*, void*, size_t, size_t>(alloc_func);

// 3. Pass the invoke function pointer + opaque userdata to C APIs
lua_setallocf(L, invoke_fptr, userdata);

// 4. Delete userdata to avoid memory leaks.
// Note: optionally use `prefix_invoker_unique` to get userdata as a unique_ptr
// and `prefix_invoker_shared` to get userdata as a shared_ptr.
delete_fptr(userdata);
```

In case the C API expects the opaque userdata as last argument instead of first, use `suffix_invoker_*` functions instead of `prefix_invoker_*`.
```cpp
// For example Box2D 3.1.0 custom filter callback.
// Note that it accepts an opaque userdata as the last parameter (suffix)
typedef bool b2CustomFilterFcn(b2ShapeId shapeIdA, b2ShapeId shapeIdB, void* context);

auto filter_callback = [](b2ShapeId shapeIdA, b2ShapeId shapeIdB) {
    /* implementation ... */
    return true;
};
auto [userdata, invoke_fptr] = functor2c::suffix_invoker_unique(filter_callback);
b2World_SetCustomFilterCallback(world_id, invoke_fptr, userdata.get());
```


## Integrating with CMake
You can integrate functor2c with CMake targets by adding a copy of this repository and linking with the `functor2c` target:
```cmake
add_subdirectory(path/to/functor2c)
target_link_libraries(my_awesome_target functor2c)
```
