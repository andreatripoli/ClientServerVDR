#define LSIZ 128 
#define RSIZ 15 
#define isCompatible(x, type) _Generic(x, type: true, default: false)
#define BUFFERT 2048