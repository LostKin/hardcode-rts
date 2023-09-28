#pragma once

#include <optional>
#include <QHostAddress>
#include <QByteArray>
#include <QNetworkDatagram>

namespace HCCN {

struct TransportMessageIdentifier {
    inline TransportMessageIdentifier (const QHostAddress& host, quint16 port, quint64 message_id);
    inline bool operator== (const TransportMessageIdentifier& b) const;

    quint8 serialized[sizeof (Q_IPV6ADDR) + sizeof (quint16) + sizeof (quint64)];
};

inline uint qHash (const TransportMessageIdentifier& key, uint seed);

} // namespace HCCN

// Implementation

inline HCCN::TransportMessageIdentifier::TransportMessageIdentifier (const QHostAddress& host, quint16 port, quint64 message_id)
{
    Q_IPV6ADDR addr = host.toIPv6Address ();
    std::memcpy (serialized, &addr, sizeof (Q_IPV6ADDR));
    std::memcpy (serialized + sizeof (Q_IPV6ADDR), &port, sizeof (quint16));
    std::memcpy (serialized + sizeof (Q_IPV6ADDR) + sizeof (quint16), &message_id, sizeof (quint64));
}
inline bool HCCN::TransportMessageIdentifier::operator== (const HCCN::TransportMessageIdentifier& b) const
{
    return !std::memcmp (serialized, b.serialized, sizeof (serialized));
}
uint HCCN::qHash (const HCCN::TransportMessageIdentifier& key, uint seed)
{
    return qHashBits (key.serialized, sizeof (key.serialized), seed);
}
