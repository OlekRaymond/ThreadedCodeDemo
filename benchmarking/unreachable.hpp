
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#   define NM_Unreachable __assume(false);
#else // GCC, Clang
#   define NM_Unreachable __builtin_unreachable();
#endif
