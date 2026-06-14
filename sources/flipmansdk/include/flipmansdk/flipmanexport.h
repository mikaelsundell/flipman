
#ifndef FLIPMANSDK_EXPORT_H
#define FLIPMANSDK_EXPORT_H

#ifdef FLIPMANSDK_STATIC_DEFINE
#    define FLIPMANSDK_EXPORT
#    define FLIPMANSDK_NO_EXPORT
#else
#    ifndef FLIPMANSDK_EXPORT
#        ifdef flipmansdk_EXPORTS
/* We are building this library */
#            define FLIPMANSDK_EXPORT __attribute__((visibility("default")))
#        else
/* We are using this library */
#            define FLIPMANSDK_EXPORT __attribute__((visibility("default")))
#        endif
#    endif

#    ifndef FLIPMANSDK_NO_EXPORT
#        define FLIPMANSDK_NO_EXPORT __attribute__((visibility("hidden")))
#    endif
#endif

#ifndef FLIPMANSDK_DEPRECATED
#    define FLIPMANSDK_DEPRECATED __attribute__((__deprecated__))
#endif

#ifndef FLIPMANSDK_DEPRECATED_EXPORT
#    define FLIPMANSDK_DEPRECATED_EXPORT FLIPMANSDK_EXPORT FLIPMANSDK_DEPRECATED
#endif

#ifndef FLIPMANSDK_DEPRECATED_NO_EXPORT
#    define FLIPMANSDK_DEPRECATED_NO_EXPORT FLIPMANSDK_NO_EXPORT FLIPMANSDK_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#    ifndef FLIPMANSDK_NO_DEPRECATED
#        define FLIPMANSDK_NO_DEPRECATED
#    endif
#endif

#endif /* FLIPMANSDK_EXPORT_H */
