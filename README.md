# Signal

**Signal** is a lightweight, header-only C++ signal-slot system with optional **asynchronous dispatch** using `std::thread`.

It allows components to communicate via events (signals), with the flexibility to run event listeners (slots) **either synchronously or asynchronously** on separate threads per handler.

---

## Features

- Header-only, just include `Signal.h`
- Thread-safe signal connections using `std::recursive_mutex`
- Run handlers **synchronously or asynchronously**
- Supports any function signature using variadic templates
- Automatically joins threads to prevent leaks
- Minimal dependencies, only the C++ standard library (C++17+)

---

## Gotchas / Important Notes

- User must maintain argument lifetime for async handlers
- One async execution per handler

### Thread Safety
Operations like `AddHandler`, `RemoveHandler`, and `Emit` are protected by a recursive mutex for safe concurrent use.

### Joining Threads
Use `TryJoinOnAll()` to wait for all running async handlers to finish before program exit.

### Handler Destruction
Removing a handler or destroying a signal will attempt to join any running async threads to prevent leaks, but long-running threads can delay destruction.

### Avoid Blocking in Async Handlers
Do not perform long blocking operations inside async handlers, as this can delay thread cleanup.

---

## Quick Example

```cpp
#include "Signal.h"
#include <iostream>
#include <thread>

int main() {
    Signal<int> sig;

    // Asynchronous handler
    sig += AsyncConnection<int>([](int x) {
        std::cout << "Async received: " << x 
                  << " on thread " << std::this_thread::get_id() << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }, true); // true = async

    // Synchronous handler
    sig += AsyncConnection<int>([](int x) {
        std::cout << "Sync received: " << x << "\n";
    }, false); // false = sync

    std::cout << "Emitting signal...\n";
    sig(42); // or sig.Emit(42);

    std::cout << "Main thread continues...\n";

    // Ensure all async threads are finished before exiting
    sig.TryJoinOnAll();

    return 0;
}
```
### CMake
cmake_minimum_required(VERSION 3.10)
project(SignalDemo)

set(CMAKE_CXX_STANDARD 17)

add_executable(demo main.cpp)

