#pragma once

#ifndef FLASHMEM
#define FLASHMEM
#endif

template<typename T>
int regular_template(T v) __attribute__((noinline));

template<typename T>
int regular_template(T v)
{
    volatile int x = (int)v;
    return x + 3;
}

template<typename T>
int flash_decl_template(T v) __attribute__((section(".flashmem"), noinline));

template<typename T>
int flash_decl_template(T v)
{
    volatile int x = (int)v;
    return x + 5;
}

template<typename T> __attribute__((section(".flashmem"), noinline))
int flash_def_template(T v)
{
    volatile int x = (int)v;
    return x + 7;
}

template<typename T>
int flashmem_named_template(T v) __attribute__((section(".flashmem.template_named"), noinline));

template<typename T>
int flashmem_named_template(T v)
{
    volatile int x = (int)v;
    return x + 11;
}

template<typename T>
int specialized_template(T v) __attribute__((noinline));

template<typename T>
int specialized_template(T v)
{
    volatile int x = (int)v;
    return x + 13;
}

template<>
FLASHMEM __attribute__((noinline))
int specialized_template<int>(int v)
{
    volatile int x = v;
    return x + 17;
}

template<typename T>
int forceinline_template(T v) __attribute__((always_inline));

template<typename T>
int forceinline_template(T v)
{
    volatile int x = (int)v;
    return x + 19;
}

FLASHMEM __attribute__((noinline))
int flash_wrapper_calls_forceinline(int v)
{
    return forceinline_template<int>(v);
}

__attribute__((section(".flashmem"), noinline))
int plain_flash(int v)
{
    volatile int x = v;
    return x + 23;
}

template<typename T>
int cold_template(T v) __attribute__((cold, noinline));

template<typename T>
int cold_template(T v)
{
    volatile int x = (int)v;
    return x + 29;
}

__attribute__((cold, noinline))
int cold_plain(int v)
{
    volatile int x = v;
    return x + 31;
}
