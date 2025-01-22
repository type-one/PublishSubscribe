# PublishSubscribe

Agnostic, Lightweight and Portable Publish-Subscribe Helper
- written in C++17 using templated classes
- synchronous/asynchronous observer
- topic subscription

Goodies:
- simple thread-safe dictionary helper on top of std::map
- simple thread-safe queue on top of std::queue
- simple waitable object on top of std::mutex and std::condition_variable
- simple non_copyable abstract class
- simple periodic task helper
- simple worker task helper
- simple thread-safe ring buffer on top of std::array
- queuable commands

https://github.com/type-one/PublishSubscribe


# What

Small test program written in C++17 to implement a simple Publish/Subscribe pattern. 
The code is portable and lightweight.

# Why

An attempt to write a flexible little framework that can be used on desktop PCs and embedded systems
(micro-computers and micro-controllers) that are able to compile and run modern C++ code.

# How

Can be compiled on Linux and Windows, and should be easily
adapted for other platforms (micro-computers, micro-controllers)

On Linux, just use cmake . -B build then go to build directory and run make (or ninja)
On Windows, just use cmake-gui to generate a Visual Studio solution

# Author

Laurent Lardinois / Type One (TFL-TDV)

https://be.linkedin.com/in/laurentlardinois

https://demozoo.org/sceners/19691/
