# CMake / Build Troubleshooting

Use this when a build is failing, behaving inconsistently between machines, or "fixed" by
deleting the build directory in a way that hasn't been explained. Diagnose before nuking.

## First, classify the failure

- **Configure-time failure** (CMake itself errors before generating build files): almost always
  a missing `find_package`, a wrong path, or a stale cache variable from a previous configure
  with different options.
- **Compile-time failure**: a real code/header problem, or a missing include path/definition
  that CMake isn't passing through correctly.
- **Link-time failure**: target dependency ordering, a library built with incompatible flags
  (e.g. one TU built without a define another expects), or a genuinely missing symbol.
- **"Works on one machine, not another"**: check for an absolute path baked into the cache, a
  system library version mismatch, or a submodule/dependency that wasn't pinned to the same
  commit.

## Before reaching for a clean rebuild

- Check `CMakeCache.txt` for the specific variable in question — a surprising number of "the
  build is haunted" issues are a stale cached value from an earlier configure that a normal
  reconfigure doesn't overwrite.
- Run the configure step with verbose output and actually read it (`cmake --trace` or
  `-DCMAKE_VERBOSE_MAKEFILE=ON` for the build step) rather than guessing at what changed.
- Check target dependency declarations (`target_link_libraries`, `add_dependencies`) match the
  actual include/link relationship — a missing explicit dependency can produce a build that
  passes on a full clean build (accidental ordering) but fails on an incremental one.
- For a submodule/vendored dependency (e.g. llama.cpp, notcurses) built as part of this project:
  confirm it's pinned to the commit you expect and was rebuilt after any bump, not silently
  reusing a stale object from before the bump.

## When a clean rebuild genuinely is the right move

- After changing compiler/toolchain, switching build type, or a CMakeLists structural change
  that the incremental system can't safely reconcile — a full `rm -rf build/` and reconfigure is
  legitimate here, not a workaround.
- Even then, note *why* the clean build was needed in the commit/PR — if it recurs, that's a
  sign of an underlying incremental-build gap worth fixing at the CMakeLists level rather than
  living with "just delete build/ if it acts up."

## What NOT to do

- Don't nuke and rebuild as the first response to a build failure — you lose the chance to
  diagnose what would otherwise recur.
- Don't add a `find_package` fallback or hardcoded path to work around a missing dependency on
  one machine without checking whether the actual fix is a documented setup step that's missing
  from onboarding.
- Don't silence a link warning about duplicate/conflicting symbols without understanding which
  definition is actually being used at runtime — silently-resolved ODR violations are a common
  source of "this only breaks in release builds."
