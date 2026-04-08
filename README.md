# PublishSubscribe

Agnostic, Lightweight and Portable Publish-Subscribe Helper

- written in C++17 (and C++20) using templated classes
- synchronous/asynchronous observer
- topic subscription

Goodies:

- simple thread-safe dictionary helper on top of std::map
- simple thread-safe queue on top of std::queue
- simple waitable object on top of std::mutex and std::condition_variable
- simple non_copyable abstract class
- simple periodic task helper
- simple worker task helper with async processing support
- simple thread-safe ring buffer on top of std::array
- simple thread-safe and resizeable ring vector on top of std::vector
- queuable commands
- lock-free ring-buffer
- custom pool allocator for global new/new[]/delete/delete[]

[GitHub repository](https://github.com/type-one/PublishSubscribe)

## What

Small test program written in C++17 (and C++20) to implement a simple Publish/Subscribe pattern. The code is portable and lightweight.

It compiles with C++17 and C++20 (with some extra features enabled, like std::ranges compatibility).

## Why

An attempt to write a flexible little framework that can be used on desktop PCs and embedded systems
(micro-computers and micro-controllers) that are able to compile and run modern C++ code.

## How

Can be compiled on Linux and Windows, and should be easily
adapted for other platforms (micro-computers, micro-controllers)

On Linux, just use `cmake . -B build` then go to build directory and run make (or ninja)
On Windows, just use `cmake-gui` to generate a Visual Studio solution

## Memory usage

On Linux you can profile the memory usage (with or without custom allocator enabled) by using valgrind:

```bash
valgrind --tool=massif ./publish_subscribe

massif-visualizer ./massif.out.xxxxxx
```

The purpose of the custom allocator is to pre-allocate tiny blocks and reuse them over the time to prevent memory fragmentation
and to minimize the need to allocate new blocks from the heap.

Indeed heavy and high frequency usage of dynamic heap allocation for events/messages can cause memory fragmentation, in particular
on platforms with a limited amount of memory available. 

More info on:

[Valgrind and Massiv](https://gist.github.com/felipeek/f9e4392cfe9a9e65dc52048e91ac58ea)
[Valgrind manual](https://valgrind.org/docs/manual/ms-manual.html)
[About custom allocators](https://www.rastergrid.com/blog/sw-eng/2021/03/custom-memory-allocators/)

## Author

Laurent Lardinois / Type One (TFL-TDV)

[LinkedIn profile](https://be.linkedin.com/in/laurentlardinois)

[Demozoo profile](https://demozoo.org/sceners/19691/)
