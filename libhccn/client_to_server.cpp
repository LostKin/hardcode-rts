#include "client_to_server.h"

#include "internal.h"


static char encode_single_message_meta (bool have_session_id, bool have_request_id)
{
    return char (uint8_t ((have_session_id ? 0x20 : 0) | (have_request_id ? 0x10 : 0)));
}
static size_t encode_fragmented_message_meta (char* encoded, bool is_tail, bool have_session_id, bool have_request_id, quint32 fragment_count, quint32 fragment_index)
{
    uint64_t fragment_number = fragment_index ? (fragment_index - 1) : (fragment_count - 1);
    uint8_t meta = (is_tail ? 0x40 : 0) | (have_session_id ? 0x20 : 0) | (have_request_id ? 0x10 : 0);
    if (fragment_number < 0x08) {
        encoded[0] = char (meta | uint8_t (fragment_number));
        return 1;
    } else if (fragment_number < 0x0408) {
        fragment_number -= 0x08;
        encoded[0] = char (meta | 0x08 | uint8_t (fragment_number >> 8));
        encoded[1] = char (uint8_t (fragment_number & 0xff));
        return 2;
    } else if (fragment_number < 0x020408) {
        fragment_number -= 0x0408;
        encoded[0] = char (meta | 0x0c | uint8_t (fragment_number >> 16));
        encoded[1] = char (uint8_t ((fragment_number >> 8) & 0xff));
        encoded[2] = char (uint8_t (fragment_number & 0xff));
        return 3;
    } else if (fragment_number < 0x01020408) {
        fragment_number -= 0x020408;
        encoded[0] = char (meta | 0x0e | uint8_t (fragment_number >> 24));
        encoded[1] = char (uint8_t ((fragment_number >> 16) & 0xff));
        encoded[2] = char (uint8_t ((fragment_number >> 8) & 0xff));
        encoded[3] = char (uint8_t (fragment_number & 0xff));
        return 4;
    } else {
        fragment_number -= 0x01020408;
        encoded[0] = char (meta | 0x0f);
        encoded[1] = char (uint8_t ((fragment_number >> 24) & 0xff));
        encoded[2] = char (uint8_t ((fragment_number >> 16) & 0xff));
        encoded[3] = char (uint8_t ((fragment_number >> 8) & 0xff));
        encoded[4] = char (uint8_t (fragment_number & 0xff));
        return 5;
    }
}
static size_t encode_ids (char* encoded, const std::optional<uint64_t>& session_id, uint64_t request_id)
{
    size_t off = session_id.has_value () ? HCCN::Internal::EncodeUint64Id (encoded, *session_id) : 0;
    return off + HCCN::Internal::EncodeUint64Id (encoded + off, request_id);
}
static bool parse_meta (const std::vector<char>& data, size_t& off, bool& is_tail, bool& session_id_present, uint64_t& fragment_number)
{
    if (data.size () < 1) {
        qDebug () << "Empty message";
        return false;
    }
    uint8_t meta = data.at (0);
    if (meta & 0x80) {
        qDebug () << "Acknowledge mode not implemented";
        return false;
    }
    if (!(meta & 0x10)) {
        qDebug () << "Missing request id";
        return false;
    }
    is_tail = meta & 0x40;
    session_id_present = meta & 0x20;
    if (!(meta & 0x08)) {
        fragment_number = meta & 0x7;
        off = 1;
    } else if (!(meta & 0x04)) {
        if (data.size () < 2) {
            qDebug () << "Unexpected end of message";
            return false;
        }
        fragment_number = ((uint64_t (meta & 0x3) << 8) | uint8_t (data.at (1))) + 0x08;
        off = 2;
    } else if (!(meta & 0x02)) {
        if (data.size () < 3) {
            qDebug () << "Unexpected end of message";
            return false;
        }
        fragment_number = ((uint64_t (meta & 0x1) << 16) | (uint64_t (uint8_t (data.at (1))) << 8) | uint8_t (data.at (2))) + 0x0408;
        off = 3;
    } else if (!(meta & 0x01)) {
        if (data.size () < 4) {
            qDebug () << "Unexpected end of message";
            return false;
        }
        fragment_number = ((uint64_t (uint8_t (data.at (1))) << 16) | (uint64_t (uint8_t (data.at (2))) << 8) | uint8_t (data.at (3))) + 0x020408;
        off = 4;
    } else {
        if (data.size () < 5) {
            qDebug () << "Unexpected end of message";
            return false;
        }
        fragment_number = ((uint64_t (uint8_t (data.at (1))) << 24) | (uint64_t (uint8_t (data.at (2))) << 16) | (uint64_t (uint8_t (data.at (3))) << 8) | uint8_t (data.at (4))) + 0x01020408;
        off = 5;
    }

    if (fragment_number >= 2097151) {
        qDebug () << "Fragment number too big" << fragment_number;
        return false;
    }

    return true;
}

namespace HCCN::ClientToServer {

Message::Message (
    const QHostAddress& host,
    uint16_t port,
    const std::optional<uint64_t>& session_id,
    uint64_t request_id,
    const std::vector<char>& message)
    : host (host)
    , port (port)
    , session_id (session_id)
    , request_id (request_id)
    , message (message)
{
}
std::vector<QNetworkDatagram> Message::encode () const
{
    char id_set[18];
    size_t id_set_len = encode_ids (id_set, session_id, request_id);
    size_t single_message_len = 1 + id_set_len + message.size ();
    std::vector<QNetworkDatagram> datagrams;
    if (single_message_len <= 508) { // Single message
        std::vector<char> encoded;
        encoded.reserve (single_message_len);
        encoded.push_back (encode_single_message_meta (session_id.has_value (), true));
        encoded.insert (encoded.end (), id_set, id_set + id_set_len);
        encoded.insert (encoded.end (), message.begin (), message.end ());
        datagrams.push_back ({QByteArray (encoded.data (), encoded.size ()), host, port});
    } else { // Fragmented
        size_t full_fragment_len = 508 - 5 - id_set_len;
        size_t full_size_fragment_count = message.size () / full_fragment_len;
        size_t last_fragment_len = message.size () % full_fragment_len;
        size_t fragment_count = full_size_fragment_count + !!last_fragment_len;
        if (fragment_count > 2097152) {
            return datagrams;
        }
        for (size_t i = 0; i < full_size_fragment_count; ++i) {
            char meta[5];
            size_t meta_len = encode_fragmented_message_meta (meta, i > 0, session_id.has_value (), true, fragment_count, i);
            std::vector<char> encoded;
            encoded.reserve (meta_len + id_set_len + full_fragment_len);
            encoded.insert (encoded.end (), meta, meta + meta_len);
            encoded.insert (encoded.end (), id_set, id_set + id_set_len);
            encoded.insert (encoded.end (), message.data () + i * full_fragment_len, message.data () + (i + 1) * full_fragment_len);
            datagrams.push_back ({QByteArray (encoded.data (), encoded.size ()), host, port});
        }
        if (last_fragment_len) {
            char meta[5];
            size_t meta_len = encode_fragmented_message_meta (meta, true, session_id.has_value (), true, fragment_count, full_size_fragment_count);
            std::vector<char> encoded;
            encoded.reserve (meta_len + id_set_len + last_fragment_len);
            encoded.insert (encoded.end (), meta, meta + meta_len);
            encoded.insert (encoded.end (), id_set, id_set + id_set_len);
            encoded.insert (encoded.end (), message.data () + full_size_fragment_count * full_fragment_len, message.data () + full_size_fragment_count * full_fragment_len + last_fragment_len);
            datagrams.push_back ({QByteArray (encoded.data (), encoded.size ()), host, port});
        }
    }
    return datagrams;
}

std::shared_ptr<MessageFragment> MessageFragment::parse (const QNetworkDatagram& datagram)
{
    // TODO: Optimize after refactoring
    std::vector<char> data = {datagram.data ().constData (), datagram.data ().constData () + datagram.data ().size ()};
    size_t off;
    bool is_tail;
    bool session_id_present;
    uint64_t fragment_number;
    if (!parse_meta (data, off, is_tail, session_id_present, fragment_number)) {
        return {nullptr};
    }
    if (data.size () < 1) {
        qDebug () << "Empty message";
        return {nullptr};
    }
    std::shared_ptr<MessageFragment> transport_message (new MessageFragment);
    transport_message->host = datagram.senderAddress ();
    transport_message->port = datagram.senderPort ();
    if (session_id_present) {
        uint64_t session_id;
        if (!HCCN::Internal::ParseUint64Id (data, off, session_id)) {
            qDebug () << "Failed to parse session id";
            return {nullptr};
        }
        transport_message->session_id = session_id;
    }
    {
        uint64_t request_id;
        if (!HCCN::Internal::ParseUint64Id (data, off, request_id)) {
            qDebug () << "Failed to parse request id";
            return {nullptr};
        }
        transport_message->request_id = request_id;
    }
    transport_message->fragment = std::vector<char> (data.data () + off, data.data () + data.size ());
    transport_message->is_tail = is_tail;
    transport_message->fragment_number = fragment_number;
    return transport_message;
}

void MessageFragmentCollector::insert (const std::shared_ptr<MessageFragment>& fragment)
{
    if (fragment->is_tail) {
        uint64_t fragment_index = fragment->fragment_number + 1;
        tail[fragment_index] = fragment;
        max_fragment_index = qMax (max_fragment_index, fragment_index);
    } else {
        head = fragment;
    }
}
bool MessageFragmentCollector::complete ()
{
    return head && head->fragment_number == max_fragment_index && max_fragment_index == uint64_t (tail.size ());
}
bool MessageFragmentCollector::valid ()
{
    return !head || max_fragment_index <= head->fragment_number;
}
std::shared_ptr<Message> MessageFragmentCollector::build ()
{
    if (!complete ())
        return {nullptr};

    std::vector<char> data = head->fragment;
    for (std::map<uint64_t, std::shared_ptr<MessageFragment>>::const_iterator it = tail.cbegin (); it != tail.cend (); ++it) {
        const std::vector<char>& fragment = it->second->fragment;
        data.insert (data.end (), fragment.begin (), fragment.end ());
    }
    return std::shared_ptr<Message> (new Message (head->host, head->port, head->session_id, head->request_id, data));
}

} // namespace HCCN::ClientToServer
