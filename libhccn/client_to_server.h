#pragma once

#include "common.h"

#include <optional>
#include <QHostAddress>
#include <QByteArray>
#include <QNetworkDatagram>

namespace HCCN::ClientToServer {

struct Message {
    Message () = default;
    Message (const QHostAddress& host, quint16 port, const std::optional<quint64>& session_id, quint64 request_id, const QByteArray& message);
    QVector<QNetworkDatagram> encode () const;

    QHostAddress host;
    quint16 port;
    std::optional<quint64> session_id;
    quint64 request_id;
    QByteArray message;
};

struct MessageFragment {
    static QSharedPointer<MessageFragment> parse (const QNetworkDatagram& datagram);

    QHostAddress host;
    quint16 port;
    std::optional<quint64> session_id;
    quint64 request_id;
    QByteArray fragment;
    bool is_tail;
    quint64 fragment_number;
};

struct MessageFragmentCollector {
public:
    void insert (const QSharedPointer<MessageFragment>& fragment);
    bool complete ();
    bool valid ();
    QSharedPointer<Message> build ();

private:
    QMap<quint64, QSharedPointer<MessageFragment>> tail;
    quint64 max_fragment_index = 0;
    QSharedPointer<MessageFragment> head;
};

} // namespace HCCN::ClientToServer
