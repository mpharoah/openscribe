#ifndef ATTRIBUTES_HPP_
#define ATTRIBUTES_HPP_

/*
 * Macros to use gcc attributes without making every function definition
 * look messy and be like a million characters long
 */

//The function should always be inlined
#if defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__)
	#define INLINE __attribute__((always_inline)) inline
#else
	#define INLINE inline
#endif

//The return value of the function should always be used
//(Produces warning if it isn't used)
#define USERET __attribute__((warn_unused_result))

//The function has no effects except for its return value, and its result
//depends only the arguments, what its pointer args point to, and global memory
// *Implies USERET*
#define PURE __attribute__((pure, warn_unused_result))

//The function has no effects except for its return value, and its result
//depends only on the arguments, but NOT data that pointer arguments point to
// *Implies USERET*
#define CONST __attribute__((const, warn_unused_result))

//The function is used frequently and should be aggressively optimized
#define HOT __attribute__((hot))

//Ensure a structure or enumeration is at minimal size
#define PACKED __attribute__((packed))

//Inline all function calls in this function if possible
#if defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__)
	#define FLATTEN __attribute__((flatten))
#else
	#define FLATTEN
#endif

//Unroll loops so long as we are compiling with -O1, -O2, or -O3
#if defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__) && !defined(IGNORE_UNROLL_ATTR)
	#define UNROLL __attribute__((optimize("unroll-loops")))
#else
	#define UNROLL
#endif



#endif /* ATTRIBUTES_HPP_ */
