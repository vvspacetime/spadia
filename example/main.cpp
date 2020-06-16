#include "peerconnection.h"
#include "server.h"
#include <glog/logging.h>
#include <memory>
#include <thread>
#include <httplib.h>

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    google::SetStderrLogging(google::INFO);
    google::InstallFailureSignalHandler();

    Server::GetInstance(8080);

    std::thread([]() {
        Server::GetInstance().Start();
    }).detach();

    httplib::Server svr;
    std::shared_ptr<MediaStream> stream;
    svr.Post("/post", [&stream](const httplib::Request& req, httplib::Response& res) {
        auto offer = req.body;
        auto pc1 = std::make_shared<PeerConnection>();
        pc1->AddRemoteStreamListener([&stream](std::shared_ptr<MediaStream> _s) {
            LOG(INFO) << "onstream";
            stream.swap(_s);
        });

        auto answer = pc1->CreateAnswer(offer);
        pc1->Start();
        std::cout << req.body << std::endl;
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(answer, "text/plain");
    });

    svr.Post("/play", [&stream](const httplib::Request& req, httplib::Response& res) {
        auto offer = req.body;
        auto pc2 = std::make_shared<PeerConnection>();
        pc2->AddStream(stream);
        auto answer = pc2->CreateAnswer(offer);
        pc2->Start();
        std::cout << req.body << std::endl;
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(answer, "text/plain");
    });

    svr.Options("/post", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content("ok", "text/plain");
    });

    svr.listen("0.0.0.0", 8080);


//    std::cout << answer << std::endl;

//    auto stream = std::make_shared<MediaStream>();
//
//    auto pc1 = new PeerConnection();
//    pc1->AddStream(stream);
//    auto offer = pc1->CreateOffer();
//
//    auto pc2 = new PeerConnection();
//    pc2->AddRemoteStreamListener([](std::shared_ptr<MediaStream> stream) {
////        onstream(stream);
//    });
//    pc2->SetRemoteSdp(offer);
//    auto answer = pc2->CreateAnswer(offer);
//    pc1->SetRemoteSdp(answer);
//
//    while (true) {};
}