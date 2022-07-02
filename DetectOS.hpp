#ifndef WORDCOUNT_DETECTOS_HPP
#define WORDCOUNT_DETECTOS_HPP

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#define ON_LINUX
#elif defined(__APPLE__) && defined(__MACH__)
#define ON_MACOS
#elif defined(_WIN32) || defined(_WIN64)
#define ON_WINDOWS
#endif

#endif // WORDCOUNT_DETECTOS_HPP
