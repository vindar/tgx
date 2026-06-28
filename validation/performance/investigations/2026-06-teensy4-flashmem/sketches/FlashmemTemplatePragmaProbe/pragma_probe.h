#pragma once

#pragma GCC section text=".flashmem"

template<typename T>
__attribute__((noinline))
int pragma_template(T v)
{
    volatile int x = (int)v;
    return x + 31;
}

__attribute__((noinline))
int pragma_plain(int v)
{
    volatile int x = v;
    return x + 37;
}

#pragma GCC section text=""
