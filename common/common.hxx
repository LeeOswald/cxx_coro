#pragma once

#define CXX_CORO_COMMON_HXX_INCLUDED 1

#define CXX_CORO_EXPORT

#include <cassert>
#include <coroutine>
#include <exception>
#include <iostream>

#include "debug.hxx"


template <typename T>
constexpr void const* Ptr(T const* p) noexcept
{
   return p;
}
