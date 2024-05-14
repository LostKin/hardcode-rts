#pragma once

#include "common.h"

#include <optional>
#include <vector>
#include <QHostAddress>
#include <QNetworkDatagram>


namespace HCCN::ServerToClient {

struct Message {
    Message () = default;
    Message (const QHostAddress& host, uint16_t port, const std::optional<uint64_t>& session_id, const std::optional<uint64_t>& request_id, uint64_t response_id, const std::vector<char>& message);
    std::vector<QNetworkDatagram> encode () const;

    QHostAddress host;
    uint16_t port;
    std::optional<uint64_t> session_id;
    std::optional<uint64_t> request_id;
    uint64_t response_id;
    std::vector<char> message;
};

struct MessageFragment {
    static std::shared_ptr<MessageFragment> parse (const QNetworkDatagram& datagram);

    QHostAddress host;
    uint16_t port;
    std::optional<uint64_t> session_id;
    std::optional<uint64_t> request_id;
    uint64_t response_id;
    std::vector<char> fragment;
    bool is_tail;
    uint64_t fragment_number;
};

struct MessageFragmentCollector {
public:
    void insert (const std::shared_ptr<MessageFragment>& fragment);
    bool complete ();
    bool valid ();
    std::shared_ptr<Message> build ();

private:
    std::map<uint64_t, std::shared_ptr<MessageFragment>> tail;
    uint64_t max_fragment_index = 0;
    std::shared_ptr<MessageFragment> head;
};

} // namespace HCCN::ServerToClient
