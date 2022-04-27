#pragma once

/*
The MIT License (MIT)

MSR Aerial Informatics and Robotics Platform
MSR Aerial Informatics and Robotics Simulator (AirSim)

Copyright (c) Microsoft Corporation

All rights reserved.

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the ""Software""), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions: The above copyright notice and this
permission notice shall be included in all copies or substantial portions of the
Software. THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifdef _MSC_VER
//'=': conversion from 'double' to 'float', possible loss of data
#define ENABLE_IGNORE_COMPILER_WARNINGS                                        \
    __pragma(warning(push)) __pragma(warning(                                  \
        disable : 4005 4100 4189 4244 4245 4239 4267 4365 4464 4456 4505 4514 4571 4624 4625 4626 4668 4701 4710 4820 4917 5026 5027 5031))
#define DISABLE_IGNORE_COMPILER_WARNINGS __pragma(warning(pop))

// TODO: limit scope of below statements required to suppress VC++ warnings
#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning(disable : 4996)
#endif

// special way to quiet the warning:  warning: format string is not a string
// literal
#ifdef __CLANG__
#define IGNORE_FORMAT_STRING_ON                                                \
    _Pragma("clang diagnostic push")                                           \
        _Pragma("clang diagnostic ignored \"-Wformat-nonliteral\"")

#define IGNORE_FORMAT_STRING_OFF _Pragma("clang diagnostic pop")
#else
#define IGNORE_FORMAT_STRING_ON
#define IGNORE_FORMAT_STRING_OFF
#endif

// Please keep this list sorted so it is easier to find stuff, also make sure there 
// is no whitespace after the trailing \, GCC doesn't like that.
#ifdef __CLANG__
#define ENABLE_IGNORE_COMPILER_WARNINGS                                           \
    _Pragma("clang diagnostic push")                                  \
    _Pragma("clang diagnostic ignored \"-Wctor-dtor-privacy\"")        \
    _Pragma("clang diagnostic ignored \"-Wdelete-non-virtual-dtor\"") \
    _Pragma("clang diagnostic ignored \"-Wmissing-field-initializers\"") \
    _Pragma("clang diagnostic ignored \"-Wold-style-cast\"")          \
    _Pragma("clang diagnostic ignored \"-Wredundant-decls\"")         \
    _Pragma("clang diagnostic ignored \"-Wreturn-type\"")             \
    _Pragma("clang diagnostic ignored \"-Wshadow\"")                  \
    _Pragma("clang diagnostic ignored \"-Wstrict-overflow\"")         \
    _Pragma("clang diagnostic ignored \"-Wswitch-default\"")          \
    _Pragma("clang diagnostic ignored \"-Wundef\"")                   \
    _Pragma("clang diagnostic ignored \"-Wunused-variable\"")         \
    _Pragma("clang diagnostic ignored \"-Wcast-qual\"")               \
    _Pragma("clang diagnostic ignored \"-Wunused-parameter\"")

/* Addition options that can be enabled
_Pragma("clang diagnostic ignored \"-Wpedantic\"")                \
_Pragma("clang diagnostic ignored \"-Wformat=\"")                 \
_Pragma("clang diagnostic ignored \"-Werror\"")                   \
_Pragma("clang diagnostic ignored \"-Werror=\"")                  \
_Pragma("clang diagnostic ignored \"-Wunused-variable\"")         \
*/

#define DISABLE_IGNORE_COMPILER_WARNINGS                                            \
    _Pragma("clang diagnostic pop")          


#else
#ifdef __GNUC__
#define ENABLE_IGNORE_COMPILER_WARNINGS                                           \
    _Pragma("GCC diagnostic push")                                  \
    _Pragma("GCC diagnostic ignored \"-Wctor-dtor-privacy\"")        \
    _Pragma("GCC diagnostic ignored \"-Wdelete-non-virtual-dtor\"") \
    _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"") \
    _Pragma("GCC diagnostic ignored \"-Wold-style-cast\"")          \
    _Pragma("GCC diagnostic ignored \"-Wredundant-decls\"")         \
    _Pragma("GCC diagnostic ignored \"-Wreturn-type\"")             \
    _Pragma("GCC diagnostic ignored \"-Wshadow\"")                  \
    _Pragma("GCC diagnostic ignored \"-Wstrict-overflow\"")         \
    _Pragma("GCC diagnostic ignored \"-Wswitch-default\"")          \
    _Pragma("GCC diagnostic ignored \"-Wundef\"")                   \
    _Pragma("GCC diagnostic ignored \"-Wunused-variable\"")         \
    _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")               \
    _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")

/* Addition options that can be enabled
_Pragma("GCC diagnostic ignored \"-Wpedantic\"")                \
_Pragma("GCC diagnostic ignored \"-Wformat=\"")                 \
_Pragma("GCC diagnostic ignored \"-Werror\"")                   \
_Pragma("GCC diagnostic ignored \"-Werror=\"")                  \
_Pragma("GCC diagnostic ignored \"-Wunused-variable\"")         \
_Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")		\
*/
#define DISABLE_IGNORE_COMPILER_WARNINGS                                            \
    _Pragma("GCC diagnostic pop")          

#endif

#endif