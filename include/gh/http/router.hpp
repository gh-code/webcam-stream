//
// Copyright (c) 2023 Gary Huang (ghuang dot nctu at gmail dot com)
//

#ifndef GH_HTTP_ROUTER_HPP
#define GH_HTTP_ROUTER_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/core/tcp_stream.hpp>

namespace gh {
namespace http {

class router
{
public:
    using Matches = std::vector<std::string>;
    using Request = boost::beast::http::request<boost::beast::http::string_body>;
    using Socket = boost::beast::tcp_stream::socket_type;
    using Callback = std::function<boost::beast::http::message_generator(Matches&& matches, Request&& req, Socket& socket)>;
    using Table = std::unordered_map<std::string, Callback>;

public:
    explicit router(boost::core::string_view name)
    : m_name(name)
    , m_view_dir("../resources/views/")
    { }

    boost::core::string_view name() const
    { return m_name; }

    template <class Callable>
    auto get(const char *path, Callable &&callback) -> void
    { table[path] = std::move(callback); }

    auto get_table() const -> const Table&
    { return table; }

    auto view(Request &request, boost::string_view name)
            -> boost::beast::http::message_generator;

private:
    std::string m_name;
    std::string m_view_dir;
    Table table;
};

} // namespace http
} // namespace gh

#endif // GH_HTTP_ROUTER_HPP