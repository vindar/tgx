#pragma once

#ifndef FLASHMEM
#define FLASHMEM
#endif

template<typename T> FLASHMEM __attribute__((noinline))
int templ_prefix(T v)
{
    volatile int x = (int)v;
    return x + 11;
}

template<typename T>
int templ_decl_suffix(T v) __attribute__((section(".flashmem"), noinline));

template<typename T>
int templ_decl_suffix(T v)
{
    volatile int x = (int)v;
    return x + 17;
}

template<typename T> __attribute__((section(".flashmem"), noinline))
int templ_prefix_direct(T v)
{
    volatile int x = (int)v;
    return x + 23;
}

template<typename T> __attribute__((section(".flashmem"), noinline, used, externally_visible))
int templ_used_direct(T v)
{
    volatile int x = (int)v;
    return x + 29;
}

FLASHMEM __attribute__((noinline))
int plain_prefix(int v)
{
    volatile int x = v;
    return x + 31;
}

int plain_decl_suffix(int v) __attribute__((section(".flashmem"), noinline));

int plain_decl_suffix(int v)
{
    volatile int x = v;
    return x + 37;
}
