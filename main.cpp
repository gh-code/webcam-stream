//
// Copyright (c) 2023 Gary Huang (ghuang dot nctu at gmail dot com)
//

#include "gh/motion_detector.hpp"
#include "gh/webcam.hpp"
#include "gh/http/server.hpp"
#include "gh/resource_manager.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <utility>

#include <cstdio>

#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/version.hpp>

auto main(/*int argc, char* argv[]*/) -> int
{
    using namespace gh::http;
    namespace http = boost::beast::http;

    // std::cout << cv::getBuildInformation();

    auto const address = "127.0.0.1";
    auto const port = static_cast<unsigned short>(8080);
    auto const doc_root = "../public";
    auto const threads = 4;
    auto const cam_index = 0;
    auto const cam_keep_on = false;

    server app{BOOST_BEAST_VERSION_STRING, threads};
    app.set_doc_root(doc_root);

    gh::motion_detector d;
    d.mark();
    gh::resource_manager<gh::webcam> cam;
    cam.set_max_shared(threads);
    cam.set_post_make_action([&d](gh::owner_ptr<gh::webcam>& cam){
        cam->install(d);
    });
    if (cam_keep_on) {
        cam.make_and_keep(cam_index);
        std::thread([&cam](){
            while (true) {
                cam.update();
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
        }).detach();
    }

    app.get("/", [&app](
            router::Matches&& /*matches*/,
            router::Request&& request,
            router::Socket& /*socket*/) {
        return app.view(request, "index");
    });

    app.get("/cam", [&app,&cam,cam_index](
            router::Matches&& matches,
            router::Request&& request,
            router::Socket& socket) -> boost::beast::http::message_generator
    {
        bool me_owner = false;
        int status = cam.make_or_reuse(me_owner, cam_index);

        if (status == 0) {
            cam.release();
            // FIXME(gh): send image with message should be better for <img>
            http::response<http::string_body> response{http::status::bad_request, request.version()};
            response.set(http::field::server, app.name());
            response.set(http::field::content_type, "text/html");
            response.keep_alive(request.keep_alive());
            response.body() = "The maximum access to the responseource was reached.";
            response.prepare_payload();
            return response;
        }
        if (status == 1) {
            printf("open webcam: %d\n", cam_index);
        }

        puts("send_stream start");

        // Source: https://github.com/boostorg/beast/issues/1740#issuecomment-922143751
        http::response<http::empty_body> response{http::status::ok, request.version()};
        response.set(http::field::server, app.name());
        response.set(http::field::cache_control, "no-cache");
        response.set(http::field::content_type, "multipart/x-mixed-replace; boundary=frame");
        response.set(http::field::expires, "0");
        response.set(http::field::pragma, "no-cache");
        response.keep_alive();
        http::response_serializer<http::empty_body> sr{response};
        http::write(socket, sr);

        boost::system::error_code ec;
        while (app.running()) {
            bool prev = me_owner;
            cam.update(me_owner);
            if (me_owner != prev) {
                puts("change owner");
            }

            auto const buffer = cam->get();
            auto const size = buffer.size();

            std::string message { "\r\n--frame\r\nContent-Type: image/jpeg" };
            message += "\r\n\r\n";
            auto bytesSent = socket.send(boost::asio::buffer(message), 0, ec);
            if (!bytesSent )
                break;
            bytesSent = socket.send(boost::asio::buffer(buffer), 0, ec);
            if (!bytesSent )
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
        socket.close();

        puts("send_stream stop");

        // Deferred releasing the real webcam
        if (cam.last()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        // Decrease reference count or release the real webcam
        if (cam.release(me_owner)) {
            puts("release webcam");
        }

        return response;
    });

    app.get("/cam/take/picture", [&app,&cam](
            router::Matches&& matches,
            router::Request&& request,
            router::Socket& /*socket*/) {
        // FIXME(gh): If one download the image and another one is taking
        // picture, the former will get wrong image.
        if (cam) {
            puts("take picture");
            cam->take_picture((app.doc_root() + "/output.jpg").c_str());
        }
        http::response<http::empty_body> response{http::status::ok, request.version()};
        response.set(http::field::server, app.name());
        response.set(http::field::connection, "close");
        return response;
    });

    // FIXME(gh): Only support 1 download at a time is not applicable.
    boost::mutex mutex;
    std::future<void> f;
    app.get("/cam/record/(\\d+)", [&app,&cam,&mutex,&f](
            router::Matches&& matches,
            router::Request&& request,
            router::Socket& /*socket*/) {
        int seconds = std::atoi(matches[1].c_str());
        if (seconds > 30) { seconds = 30; }
        if (seconds > 0 && cam) {
            if (mutex.try_lock()) {
                cam->record_video((app.doc_root() + "/live001.avi").c_str());
                /*f = */std::async(std::launch::async, [&app,&cam,&mutex,seconds](){
                    std::cout << "record video for " << seconds << " seconds" << '\n';
                    std::this_thread::sleep_for(std::chrono::seconds{seconds});
                    cam->stop_record();
                    // FIXME(gh): If live.avi is opened for downloading, this 
                    // moving file will fail.
                    ::rename((app.doc_root() + "/live001.avi").c_str(),
                             (app.doc_root() + "/live.avi").c_str());
                    puts("recording is done");
                    mutex.unlock();
                });
            }
        }
        http::response<http::empty_body> response{http::status::ok, request.version()};
        response.set(http::field::server, app.name());
        response.set(http::field::connection, "close");
        return response;
    });

    app.run(address, port);

    puts("exit gracefully");

    return 0;
}
