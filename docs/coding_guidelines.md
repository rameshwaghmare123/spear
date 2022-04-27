# Coding Guidelines

We generally adhere to the [AirSim Coding Guidelines](https://microsoft.github.io/AirSim/coding_guidelines/) except as noted below. In cases where the AirSim guidelines don't make a recommendation, we generally agree with the recommendations in the [ISO C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) except as noted below. The AirSim guidelines are mostly a style guide (e.g., how to name variables), whereas the ISO C++ guidelines give more substantial recommendations on how and when to use language features (e.g., how to make effective use of move semantics).

We have implemented these guidelines recently, and there is a significant amount of non-compliant code in this repository. We are therefore adopting a per-file incremental strategy to bring our code into compliance. If you make any changes to a file, then edit the entire file so it conforms to these guidelines.

## Our design philosophy

- Prefer code that is as small and simple as possible
- Only implement functionality that we know we need
- Be as consistent as possible with everything (e.g., data flow, interface design, naming conventions, documentation, etc)
- Avoid trivial pass-through layers
- Avoid excessive folder hierarchy
- System behavior should be as symmetric as possible (e.g., if there is an `Initialize()` method, there should also be a `Terminate()` method that returns the system into the state it was in before being initialized)
- System behavior should be as stateless as possible
- Interfaces should be as narrow as possible
- Classes, functions, and variables should be as private and as local as possible
- Integrate code into the `main` branch as frequently as possible (e.g., in one-week intervals or less)
- Clean and re-factor as you go

## Our guidelines

The guidelines below may disagree with the AirSim and ISO C++ guidelines, but we adhere to them anyway.

### General

**Use asserts instead of throwing exceptions.** We want to catch all errors as soon as they happen, without giving surrounding code an opportunity to continue executing. In C++, always use our `ASSERT` macro. Don't use Unreal Engine's `check` macro, because it is hard to enable `check` in Shipping builds.

```python
# bad
if world is None
    throw Exception()

# good
assert world is not None
```

```cpp
// bad
if (!world) {
    throw std::exception();
}

// bad 
check(world);

// good
ASSERT(world);
```

**Use your own human judgement rather than `clang-format` and `black` to format your code.** `clang-format` is not idempotent, can get stuck in cycles, and requires a lot of configuration to behave sensibly. Likewise, `black` requires a lot of configuration to behave sensibly.

### Python

**Use the following naming conventions.** We need a naming convention for our Python code, and this one roughly agrees with the rest of the scientific Python stack (e.g., NumPy, SciPy, matplotlib, Pandas, PyTorch, etc).

- `leading_lowercase_underscore` for variables, functions, methods, modules, and filenames
- `LeadingUppercaseCamelCase` for classes
- `self._leading_underscore` for member variables and methods that are intended to be private
- Spaces instead of tabs with four spaces per indent

**Don't use type annotations.** It's easy to forget to update these annotations as the code changes, in which case they become actively misleading. In some cases, an object you get from a third-party library can be one of several types, leading to an annotation that is either wrong (and therefore misleading) or excessively verbose. These annotations also interfere with readability and expressiveness, and this project is too small to benefit significantly from their use.

```python
# bad
def add(x: Union[int, List[int], float, List[float], y: Union[int, List[int], float, List[float]]) -> Union[int, float]:
    if isinstance(x, list):
        x_: Union[int, float] = reduce(sum, x)
    if isinstance(y, list):
        y_: Union[int, float] = reduce(sum, y)        
    sum: Union[int, float] = x_ + y_
    return sum

# good
def add(x, y):
    if isinstance(x, list):
        x = reduce(sum, x)
    if isinstance(y, list):
        y = reduce(sum, y)        
    sum = x + y
    return sum
```

**Don't use docstrings (for now).** Docstrings are similar to public-facing README documents. They are useful when an interface has completely stabilized, and that interface needs to be communicated to novice users. Neither of these conditions currently applies to this project, and docstrings are therefore not worth the maintenance effort. This may change in future as our Python interfaces stabilize and we get closer to a public release.

```python
# bad: too much maintenance effort

class Photo(ndarray):
    """
    Array with associated photographic information.

    Attributes
    ----------
    exposure : float
        Exposure in seconds.

    Methods
    -------
    colorspace(c='rgb')
        Represent the photo in the given colorspace.
    gamma(n=1.0)
        Change the photo's gamma exposure.
    """

# good: minimal maintenance effort

# array with associated photographic information
class Photo(ndarray):
```

### C++

**Use braces even for one-line if statements.** It's easy to forget to add braces when adding code to the body of the if statement.

```cpp
// bad
if (x < 0)
  break;

// good
if (x < 0) {
  break;
}
```

**Use `#pragma once`.** It's easy to forget to rename `PATH_TO_FILE_H` if you rename or move `file.h`

```cpp
// bad
#ifndef PATH_TO_FILE_H
#define PATH_TO_FILE_H
// ...
#endif

// good
#pragma once
// ...
```

**Don't use `/* */` comments.** They can't be nested recursively, whereas `//` comments can be nested recursively. Therefore `//` comments make it easier to comment and uncomment big chunks of code.

```cpp
/* bad */
i += 1;

// good
i += 1;
```