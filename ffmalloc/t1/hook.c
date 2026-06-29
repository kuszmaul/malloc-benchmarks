#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>

// Bind our custom function to the versioned symbol requested by the app
// Use @@ for a default version or @ for a non-default/specific version
__asm__(".symver my_realpath, realpath@GLIBC_2.3");

char *my_realpath(const char *path, char *resolved_path) {
    printf("[Hooked!] Intercepted realpath for: %s\n", path);

    // To call the original function, fetch it explicitly by its version
    // using dlvsym() instead of dlsym()
    char *(*original_realpath)(const char *, char *);
    original_realpath = dlvsym(RTLD_NEXT, "realpath", "GLIBC_2.3");

    return original_realpath(path, resolved_path);
}
