# Commodities AllocCxStructure host regression

Copyright (C) 2026, The AROS Development Team. All rights reserved.

This fixture compiles the real `Extensions[]` table and `AllocCxStructure()`
function extracted from `workbench/libs/commodities/eventfuncs.c` with `#line`
mapping preserved. It does not maintain a duplicate copy of the allocation or
initialization logic.

From the repository root, run:

```
make -C developer/debug/test/commodities_host
make -C developer/debug/test/commodities_host test-sanitize
```

Only Python 3 and a C99 host compiler are required. `AROS_ROOT`, `BUILD_DIR`,
`CC`, and `PYTHON` can be overridden; no AROS SDK is required. The sanitizer
target enables AddressSanitizer and UndefinedBehaviorSanitizer.

The scenarios verify:

* `CX_OBJECT` when `AllocVec()` fails across all object types (`CX_INVALID`
  through `CX_ZERO`, including `CX_FILTER` and `CX_BROKER`): returns `NULL`,
  performs no write, and leaves nothing allocated.
* `CX_OBJECT` when `AllocVec()` succeeds across all object types: allocates
  `sizeof(CxObj) + Extensions[objtype]` with `MEMF_CLEAR | MEMF_PUBLIC`, points
  `co_Ext.co_FilterIX` to `co + 1`, initializes `co_ObjList` as an empty list,
  and sets `co_Node.ln_Name` to `bext_Name` only for `CX_BROKER`.
* `CX_MESSAGE` (`CXM_SINGLE` and `CXM_DOUBLE`) and `CX_INPUTEVENT` under both
  allocation failure (including second-stage `CXM_DOUBLE` rollback) and success.

Each scenario prints a `FAIL` line for a check that fails, and the last line is
`Commodities AllocCxStructure: <n> checks, <n> failed`. On a tree whose
`AllocCxStructure()` uses the object before testing it, the fixture ends on a
signal at the first failed allocation, and the runner says so.
