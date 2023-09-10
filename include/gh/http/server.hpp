//
// Copyright (c) 2023 Gary Huang (ghuang dot nctu at gmail dot com)
//

#ifndef GH_HTTP_SERVER_HPP
#define GH_HTTP_SERVER_HPP

#include "gh/http/router.hpp"

namespace gh {
namespace http {

class server : public router
{
public:
    server(boost::core::string_view name, int threads=1)
    : router(name)
    , m_threads(threads)
    , m_doc_root("../public")
    { }

    auto run(const char* host="127.0.0.1", unsigned short port=5000) -> int;

    auto stop() -> void
    { m_stop = true; }

    auto running() const -> bool
    { return !m_stop; }

    auto stopped() const -> bool
    { return m_stop; }

    auto threads() const -> int
    { return m_threads; }

    auto set_doc_root(const char* path) -> void
    { m_doc_root = path; }

    auto doc_root() const -> std::string
    { return m_doc_root; }

protected:
    int m_threads;
    bool m_stop;
    std::string m_doc_root;
};

} // namespace http
} // namespace gh

#endif // GH_HTTP_SERVER_HPP