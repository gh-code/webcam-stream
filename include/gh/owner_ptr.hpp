//
// Copyright (c) 2023 Gary Huang (ghuang dot nctu at gmail dot com)
//

#ifndef GH_OWNER_PTR
#define GH_OWNER_PTR

#include <memory>
#include <cstdio>

namespace gh {

template<class T, class Deleter = std::default_delete<T>>
class owner_ptr
{
public:
    using pointer = T*;

    owner_ptr()
    : m_max_shared(-1)
    , m_shared(0)
    , m_owned(false)
    {}

    owner_ptr(std::nullptr_t)
    : m_max_shared(-1)
    , m_shared(0)
    , m_owned(false)
    {}

    explicit owner_ptr(pointer p) noexcept
    : m_max_shared(-1)
    , m_shared(1)
    , m_owned(true)
    , m_ptr(p)
    {}

    owner_ptr(const owner_ptr&) = delete;
    owner_ptr& operator=(const owner_ptr&) = delete;

    explicit operator bool() const noexcept
    { return static_cast<bool>(m_ptr); }

    typename std::add_lvalue_reference<T>::type
    operator*() const noexcept(noexcept(*std::declval<pointer>()))
    { return *m_ptr; }

    auto operator->() const noexcept -> pointer
    { return m_ptr.operator->(); }

    owner_ptr(owner_ptr&&) = default;
    owner_ptr &operator=(owner_ptr&& other)
    {
        swap(*this, other);
        return *this;
    }

    auto set_max_shared(int n) noexcept -> void
    { m_max_shared = n; }

    auto use() noexcept -> void
    { ++m_shared; }

    auto last() const noexcept -> bool
    { return (m_shared == 1); }

    auto release() noexcept -> pointer
    {
        --m_shared;
        return (m_shared == 0) ? m_ptr.release() : nullptr;
    }

    auto reset() noexcept -> void
    { m_owned = false; }

    auto reached() const noexcept -> bool
    {
        if (m_max_shared <= 0) { return false; }
        return m_shared >= m_max_shared;
    }

    auto try_own() noexcept -> bool
    { 
        if (m_owned) { return false; }
        m_owned = true;
        return true;
    }

    friend auto swap(owner_ptr& a, owner_ptr& b) -> void
    {
        using std::swap;
        swap(a.m_max_shared, b.m_max_shared);
        swap(a.m_shared, b.m_shared);
        swap(a.m_owned, b.m_owned);
        swap(a.m_ptr, b.m_ptr);
    }

private:
    int m_max_shared;
    int m_shared;
    bool m_owned;
    std::unique_ptr<T> m_ptr;
};

template<class T, class... Args>
auto make_owner(Args&&... args) -> owner_ptr<T>
{ return owner_ptr<T>(new T(args)...); }

} // namespace gh

#endif // GH_OWNER_PTR