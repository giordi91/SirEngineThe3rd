// All STL usage is through these defined types.
// You can use different types with the same interfaces used if you'd like to avoid STL.

#define DFS_LOG(...) printf(__VA_ARGS__);

#ifndef TDYNAMICARRAY
#define TDYNAMICARRAY std::vector
#endif

#ifndef TSTATICARRAY
#define TSTATICARRAY std::array
#endif

#ifndef TSTRING
#define TSTRING std::string
#endif

#ifndef TDYNAMICARRAY_SIZE
#define TDYNAMICARRAY_SIZE(x) x.size()
#endif

#ifndef TDYNAMICARRAY_RESIZE
#define TDYNAMICARRAY_RESIZE(x,y) x.resize(y)
#endif

#ifndef TDYNAMICARRAY_PUSHBACK
#define TDYNAMICARRAY_PUSHBACK(x,y) x.push_back(y)
#endif

#ifndef TSTRING_RESIZE
#define TSTRING_RESIZE(x,y) x.resize(y)
#endif