# Build Fixes - January 23, 2026

This document summarizes the build fixes applied to resolve compilation errors.

## Issue 1: C++20 Static Assert Compatibility

### Problem

Multiple compilation errors occurred in the reflection/serialization system:

```
error C2338: static_assert failed: 'No matching overload! Define your own custom bindings for the type'
error C2338: static_assert failed: 'Arity unsupported! Update MakeAggregateBindings(Arity, ...) with another macro to support your arity'
error C2607: static assertion failed (in archive.hpp and serializer.hpp)
```

### Root Cause

In C++20 with stricter compliance, `static_assert(std::false_type::value, ...)` triggers immediately during template parsing because `std::false_type::value` is **not dependent on template parameters**. This caused compilation failures in files that merely included the headers, even without instantiating the problematic templates.

The pattern `static_assert(std::false_type::value, "message")` was historically used to create "always fail" assertions that only trigger when a template is instantiated. However, modern C++20 compilers (including MSVC) evaluate this immediately since the expression doesn't depend on any template parameter.

### Solution

Changed all `static_assert(std::false_type::value, ...)` patterns to use expressions that are **dependent on template parameters**.

### Files Modified

#### 1. `lib/resource_pipeline/core/include/rsc-core/reflection/bindings.hpp`

**Change 1 - EnumBindings template (line 19):**
```cpp
// Before:
static_assert(std::false_type::value, "No matching overload! Define your own custom bindings for the type");

// After:
static_assert(sizeof(EnumType) == 0, "No matching overload! Define your own custom bindings for the type");
```

**Change 2 - aggregate_binding_helper template (line 65):**
```cpp
// Before:
static_assert(std::false_type::value, "Arity unsupported! Update MakeAggregateBindings(Arity, ...) with another macro to support your arity");

// After:
static_assert(sizeof(Type) == 0, "Arity unsupported! Update MakeAggregateBindings(Arity, ...) with another macro to support your arity");
```

#### 2. `lib/resource_pipeline/core/include/rsc-core/serialization/archive.hpp`

**Change - in_archive and out_archive templates (lines 12, 17):**
```cpp
// Before:
template <utility::static_string format_name>
struct in_archive {
    static_assert(std::false_type::value && "unsupported archive format! ...");
};

template <utility::static_string format_name>
struct out_archive {
    static_assert(std::false_type::value && "unsupported archive format! ...");
};

// After:
template <utility::static_string format_name>
struct in_archive {
    static_assert(format_name.m_data[0] == '\0' && false, "unsupported archive format! ...");
};

template <utility::static_string format_name>
struct out_archive {
    static_assert(format_name.m_data[0] == '\0' && false, "unsupported archive format! ...");
};
```

#### 3. `lib/resource_pipeline/core/include/rsc-core/serialization/serializer.hpp`

**Change - serializer template (line 11):**
```cpp
// Before:
template <rp::utility::static_string format>
struct serializer {
    static_assert(std::false_type::value && "unknown serializer, ...");
};

// After:
template <rp::utility::static_string format>
struct serializer {
    static_assert(format.m_data[0] == '\0' && false, "unknown serializer, ...");
};
```

### Why These Fixes Work

- `sizeof(EnumType) == 0` and `sizeof(Type) == 0` are dependent on template parameters, so they're only evaluated when the template is instantiated with concrete types.
- `format_name.m_data[0] == '\0' && false` is dependent on the non-type template parameter `format_name`, achieving the same deferred evaluation.

---

## Issue 2: Windows ERROR Macro Conflict

### Problem

Compilation errors in the resource compiler tool:

```
error C2589: 'constant': illegal token on right side of '::'
error C2062: type 'unknown-type' unexpected
```

These errors occurred at lines using `CommandProcessor::Status::ERROR` and `CommandProcessor::SEVERITY::ERROR`.

### Root Cause

Windows headers (specifically `wingdi.h`) define `ERROR` as a macro:
```cpp
#define ERROR 0
```

This macro expansion transforms code like:
```cpp
CommandProcessor::Status::ERROR
```
Into:
```cpp
CommandProcessor::Status::0  // Invalid syntax!
```

### Solution

Added `#undef ERROR` after the includes in the affected header file.

### File Modified

#### `tools/resource_compiler/app/cli/include/command.h`

```cpp
// Added after includes, before struct definition:

// Windows headers define ERROR as a macro, which conflicts with our enum values
#ifdef ERROR
#undef ERROR
#endif
```

### Alternative Solutions (Not Applied)

1. **Rename enum values**: Use `Error` instead of `ERROR` (requires more changes)
2. **Define NOMINMAX and WIN32_LEAN_AND_MEAN**: Add before Windows headers to reduce macro pollution
3. **Use fully qualified names with parentheses**: `(CommandProcessor::Status::ERROR)` (doesn't always work)

---

## Summary

| Issue | Files Changed | Fix Type |
|-------|---------------|----------|
| C++20 static_assert compatibility | `bindings.hpp`, `archive.hpp`, `serializer.hpp` | Make static_assert dependent on template parameters |
| Windows ERROR macro conflict | `command.h` | `#undef ERROR` |

## Prevention

To avoid similar issues in the future:

1. **For template static_asserts**: Always use expressions dependent on template parameters (e.g., `sizeof(T) == 0` instead of `std::false_type::value`)

2. **For Windows macro conflicts**: Consider adding to a common header:
   ```cpp
   #ifdef _WIN32
   #define NOMINMAX
   #define WIN32_LEAN_AND_MEAN
   #endif
   ```
   And selectively `#undef` problematic macros like `ERROR`, `min`, `max`, `near`, `far`, etc.
