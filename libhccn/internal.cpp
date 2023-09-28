#include "internal.h"

namespace HCCN::Internal {

bool ParseUint64Id (const QByteArray& data, qsizetype& off, quint64& value)
{
    const quint8* rest = (const quint8*) data.data () + off;
    qsizetype rest_size = data.size () - off;
    if (rest_size < 1)
        return false;
    quint8 session_id_head = rest[0];
    if (!(session_id_head & 0x80)) { // 0xxxxxxx
        value = session_id_head;
        off += 1;
    } else if (!(session_id_head & 0x40)) { // 10xxxxxx xxxxxxxx
        if (rest_size < 2)
            return false;
        value = ((quint64 (session_id_head & 0x3f) << 8) | quint64 (rest[1])) + 0x80ULL;
        off += 2;
    } else if (!(session_id_head & 0x20)) { // 110xxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 3)
            return false;
        value = ((quint64 (session_id_head & 0x1f) << 16) | (quint64 (rest[1]) << 8) | quint64 (rest[2])) + 0x4080ULL;
        off += 3;
    } else if (!(session_id_head & 0x10)) { // 1110xxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 4)
            return false;
        value = ((quint64 (session_id_head & 0x0f) << 24) | (quint64 (rest[1]) << 16) | (quint64 (rest[2]) << 8) | quint64 (rest[3])) + 0x204080ULL;
        off += 4;
    } else if (!(session_id_head & 0x08)) { // 11110xxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 5)
            return false;
        value = ((quint64 (session_id_head & 0x07) << 32) | (quint64 (rest[1]) << 24) | (quint64 (rest[2]) << 16) | (quint64 (rest[3]) << 8) | quint64 (rest[4])) + 0x10204080ULL;
        off += 5;
    } else if (!(session_id_head & 0x04)) { // 111110xx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 6)
            return false;
        value =
            ((quint64 (session_id_head & 0x03) << 40) | (quint64 (rest[1]) << 32) | (quint64 (rest[2]) << 24) | (quint64 (rest[3]) << 16) |
             (quint64 (rest[4]) << 8) | quint64 (rest[5])) +
            0x0810204080ULL;
        off += 6;
    } else if (!(session_id_head & 0x02)) { // 1111110x xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 7)
            return false;
        value =
            ((quint64 (session_id_head & 0x01) << 48) | (quint64 (rest[1]) << 40) | (quint64 (rest[2]) << 32) | (quint64 (rest[3]) << 24) |
             (quint64 (rest[4]) << 16) | (quint64 (rest[5]) << 8) | quint64 (rest[6])) +
            0x040810204080ULL;
        off += 7;
    } else if (!(session_id_head & 0x01)) { // 11111110 xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 8)
            return false;
        value =
            ((quint64 (rest[1]) << 48) | (quint64 (rest[2]) << 40) | (quint64 (rest[3]) << 32) | (quint64 (rest[4]) << 24) |
             (quint64 (rest[5]) << 16) | (quint64 (rest[6]) << 8) | quint64 (rest[7])) +
            0x02040810204080ULL;
        off += 8;
    } else { // 11111111 xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 9)
            return false;
        value =
            ((quint64 (rest[1]) << 56) | (quint64 (rest[2]) << 48) | (quint64 (rest[3]) << 40) | (quint64 (rest[4]) << 32) |
             (quint64 (rest[5]) << 24) | (quint64 (rest[6]) << 16) | (quint64 (rest[7]) << 8) | quint64 (rest[8]));
        if (value >= 0xFEFDFBF7EFDFBF80ULL)
            return false;
        value += 0x0102040810204080ULL;
        off += 9;
    }
    return true;
}
size_t EncodeUint64Id (char* encoded, quint64 id)
{
    if (id < 0x80ULL) {
        encoded[0] = char (id);
        return 1;
    } else if (id < 0x4080ULL) {
        id -= 0x80ULL;
        encoded[0] = (id >> 8) | 0x80;
        encoded[1] = id & 0xff;
        return 2;
    } else if (id < 0x204080ULL) {
        id -= 0x4080ULL;
        encoded[0] = (id >> 16) | 0xc0;
        encoded[1] = (id >> 8) & 0xff;
        encoded[2] = id & 0xff;
        return 3;
    } else if (id < 0x10204080ULL) {
        id -= 0x204080ULL;
        encoded[0] = (id >> 24) | 0xe0;
        encoded[1] = (id >> 16) & 0xff;
        encoded[2] = (id >> 8) & 0xff;
        encoded[3] = id & 0xff;
        return 4;
    } else if (id < 0x0810204080ULL) {
        id -= 0x10204080ULL;
        encoded[0] = (id >> 32) | 0xf0;
        encoded[1] = (id >> 24) & 0xff;
        encoded[2] = (id >> 16) & 0xff;
        encoded[3] = (id >> 8) & 0xff;
        encoded[4] = id & 0xff;
        return 5;
    } else if (id < 0x040810204080ULL) {
        id -= 0x0810204080ULL;
        encoded[0] = (id >> 40) | 0xf8;
        encoded[1] = (id >> 32) & 0xff;
        encoded[2] = (id >> 24) & 0xff;
        encoded[3] = (id >> 16) & 0xff;
        encoded[4] = (id >> 8) & 0xff;
        encoded[5] = id & 0xff;
        return 6;
    } else if (id < 0x02040810204080ULL) {
        id -= 0x040810204080ULL;
        encoded[0] = (id >> 48) | 0xfc;
        encoded[1] = (id >> 40) & 0xff;
        encoded[2] = (id >> 32) & 0xff;
        encoded[3] = (id >> 24) & 0xff;
        encoded[4] = (id >> 16) & 0xff;
        encoded[5] = (id >> 8) & 0xff;
        encoded[6] = id & 0xff;
        return 7;
    } else if (id < 0x0102040810204080ULL) {
        id -= 0x02040810204080ULL;
        encoded[0] = (id >> 56) | 0xfe;
        encoded[1] = (id >> 48) & 0xff;
        encoded[2] = (id >> 40) & 0xff;
        encoded[3] = (id >> 32) & 0xff;
        encoded[4] = (id >> 24) & 0xff;
        encoded[5] = (id >> 16) & 0xff;
        encoded[6] = (id >> 8) & 0xff;
        encoded[7] = id & 0xff;
        return 8;
    } else {
        id -= 0x0102040810204080ULL;
        encoded[0] = 0xff;
        encoded[1] = (id >> 56) & 0xff;
        encoded[2] = (id >> 48) & 0xff;
        encoded[3] = (id >> 40) & 0xff;
        encoded[4] = (id >> 32) & 0xff;
        encoded[5] = (id >> 24) & 0xff;
        encoded[6] = (id >> 16) & 0xff;
        encoded[7] = (id >> 8) & 0xff;
        encoded[8] = id & 0xff;
        return 9;
    }
}

} // namespace HCCN::Internal
