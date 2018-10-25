# Aws

This is a library which contains utilities useful for interating with Amazon
Web Services (AWS).

## Usage

The `Aws::SignApi` functions can be used to sign AWS Application Programming
Interface (API) request messages.

## Supported platforms / recommended toolchains

This is a portable C++11 application which depends only on the C++11 compiler,
the C and C++ standard libraries, and other C++11 libraries with similar
dependencies, so it should be supported on almost any platform.  The following
are recommended toolchains for popular platforms.

* Windows -- [Visual Studio](https://www.visualstudio.com/) (Microsoft Visual
  C++)
* Linux -- clang or gcc
* MacOS -- Xcode (clang)

## Building

This library is not intended to stand alone.  It is intended to be included in
a larger solution which uses [CMake](https://cmake.org/) to generate the build
system and build applications which will link with the library.

There are two distinct steps in the build process:

1. Generation of the build system, using CMake
2. Compiling, linking, etc., using CMake-compatible toolchain

### Prerequisites

* [CMake](https://cmake.org/) version 3.8 or newer
* C++11 toolchain compatible with CMake for your development platform (e.g.
  [Visual Studio](https://www.visualstudio.com/) on Windows)
* [Hash](https://github.com/rhymu8354/Hash.git) - a library which implements
  various cryptographic hash and message digest functions.
* [Http](https://github.com/rhymu8354/Http.git) - a library which implements
  [RFC 7230](https://tools.ietf.org/html/rfc7230), "Hypertext Transfer Protocol
  (HTTP/1.1): Message Syntax and Routing".
* [Json](https://github.com/rhymu8354/Json.git) - a library which implements
  [RFC 7159](https://tools.ietf.org/html/rfc7159), "The JavaScript Object
  Notation (JSON) Data Interchange Format".
* [MessageHeaders](https://github.com/rhymu8354/MessageHeaders.git) - a library
  which can parse and generate e-mail or web message headers
* [SystemAbstractions](https://github.com/rhymu8354/SystemAbstractions.git) - a
  cross-platform adapter library for system services whose APIs vary from one
  operating system to another
* [Uri](https://github.com/rhymu8354/Uri.git) - a library that can parse and
  generate Uniform Resource Identifiers (URIs)

### Build system generation

Generate the build system using [CMake](https://cmake.org/) from the solution
root.  For example:

```bash
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A "x64" ..
```

### Compiling, linking, et cetera

Either use [CMake](https://cmake.org/) or your toolchain's IDE to build.
For [CMake](https://cmake.org/):

```bash
cd build
cmake --build . --config Release
```
