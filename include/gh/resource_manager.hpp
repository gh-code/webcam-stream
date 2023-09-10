//
// Copyright (c) 2023 Gary Huang (ghuang dot nctu at gmail dot com)
//

#ifndef GH_RESOURCE_MANAGER
#define GH_RESOURCE_MANAGER

#include "gh/owner_ptr.hpp"

#include <boost/thread/mutex.hpp>

namespace gh {

template<class T>
class resource_manager
{
public:
    using pointer = T*;

    resource_manager()
    : m_max_shared{-1}
    { }

    resource_manager(const resource_manager&) = delete;
    resource_manager& operator=(const resource_manager&) = delete;

    explicit operator bool() const noexcept
    {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        return static_cast<bool>(m_ptr);
    }

    typename std::add_lvalue_reference<T>::type
    operator*() const noexcept(noexcept(*std::declval<pointer>()))
    {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        return *m_ptr;
    }

    auto operator->() const noexcept -> pointer
    {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        return m_ptr.operator->();
    }

    auto set_max_shared(int n) -> void
    { m_max_shared = n; }

    template <class Callable>
    auto set_post_make_action(Callable&& callback) -> void
    { m_callback = std::move(callback); }

    template<class... Args>
    auto make_without_lock(Args&&... args) -> void
    {
        m_ptr = gh::make_owner<T, Args...>(std::forward<Args...>(args)...);
        m_ptr.set_max_shared(m_max_shared);
        if (m_callback) {
            m_callback(m_ptr);
        }
    }

    template<class... Args>
    auto make(Args&&... args) -> void {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        make_without_lock(args...);
    }

    template<class... Args>
    auto make_or_reuse(bool& me_owner, Args&&... args) -> int {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        if (m_ptr) {
            m_ptr.use();
        } else {
            make_without_lock(args...);
            me_owner = true;
        }
        return m_ptr.reached() ? 0 : (me_owner ? 1 : 2);
    }

    template<class... Args>
    auto make_and_keep(Args&&... args) -> void {
        ++m_max_shared;
        make(args...);
        --m_max_shared;
    }

    auto update(bool& me_owner) -> void {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        if (m_ptr.try_own()) {
            me_owner = true;
        }
        if (me_owner) {
            m_ptr->update();
        }
    }

    auto update() -> void
    {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        m_ptr->update();
    }

    auto last() const -> bool {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        return m_ptr.last();
    }

    auto release(bool& me_owner) -> int {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        if (me_owner) {
            m_ptr.reset();
        }
        auto p = m_ptr.release();
        if (p) {
            delete p;
            return 1;
        }
        return 0;
    }

    auto release() -> pointer {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        return m_ptr.release();
    }

private:
    int m_max_shared;
    owner_ptr<T> m_ptr;
    mutable boost::mutex m_mutex;
    std::function<void(owner_ptr<T>&)> m_callback;
};

} // namespace gh

#endif // GH_RESOURCE_MANAGER