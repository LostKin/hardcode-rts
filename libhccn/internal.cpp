#include "internal.h"

namespace HCCN::Internal {

bool ParseUint64Id (const std::vector<char>& data, size_t& off, uint64_t& value)
{
    const uint8_t* rest = (const uint8_t*) data.data () + off;
    size_t rest_size = data.size () - off;
    if (rest_size < 1)
        return false;
    uint8_t session_id_head = rest[0];
    if (!(session_id_head & 0x80)) { // 0xxxxxxx
        value = session_id_head;
        off += 1;
    } else if (!(session_id_head & 0x40)) { // 10xxxxxx xxxxxxxx
        if (rest_size < 2)
            return false;
        value = ((uint64_t (session_id_head & 0x3f) << 8) | uint64_t (rest[1])) + 0x80ULL;
        off += 2;
    } else if (!(session_id_head & 0x20)) { // 110xxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 3)
            return false;
        value = ((uint64_t (session_id_head & 0x1f) << 16) | (uint64_t (rest[1]) << 8) | uint64_t (rest[2])) + 0x4080ULL;
        off += 3;
    } else if (!(session_id_head & 0x10)) { // 1110xxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 4)
            return false;
        value = ((uint64_t (session_id_head & 0x0f) << 24) | (uint64_t (rest[1]) << 16) | (uint64_t (rest[2]) << 8) | uint64_t (rest[3])) + 0x204080ULL;
        off += 4;
    } else if (!(session_id_head & 0x08)) { // 11110xxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 5)
            return false;
        value = ((uint64_t (session_id_head & 0x07) << 32) | (uint64_t (rest[1]) << 24) | (uint64_t (rest[2]) << 16) | (uint64_t (rest[3]) << 8) | uint64_t (rest[4])) + 0x10204080ULL;
        off += 5;
    } else if (!(session_id_head & 0x04)) { // 111110xx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 6)
            return false;
        value =
            ((uint64_t (session_id_head & 0x03) << 40) | (uint64_t (rest[1]) << 32) | (uint64_t (rest[2]) << 24) | (uint64_t (rest[3]) << 16) |
             (uint64_t (rest[4]) << 8) | uint64_t (rest[5])) +
            0x0810204080ULL;
        off += 6;
    } else if (!(session_id_head & 0x02)) { // 1111110x xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 7)
            return false;
        value =
            ((uint64_t (session_id_head & 0x01) << 48) | (uint64_t (rest[1]) << 40) | (uint64_t (rest[2]) << 32) | (uint64_t (rest[3]) << 24) |
             (uint64_t (rest[4]) << 16) | (uint64_t (rest[5]) << 8) | uint64_t (rest[6])) +
            0x040810204080ULL;
        off += 7;
    } else if (!(session_id_head & 0x01)) { // 11111110 xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 8)
            return false;
        value =
            ((uint64_t (rest[1]) << 48) | (uint64_t (rest[2]) << 40) | (uint64_t (rest[3]) << 32) | (uint64_t (rest[4]) << 24) |
             (uint64_t (rest[5]) << 16) | (uint64_t (rest[6]) << 8) | uint64_t (rest[7])) +
            0x02040810204080ULL;
        off += 8;
    } else { // 11111111 xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (rest_size < 9)
            return false;
        value =
            ((uint64_t (rest[1]) << 56) | (uint64_t (rest[2]) << 48) | (uint64_t (rest[3]) << 40) | (uint64_t (rest[4]) << 32) |
             (uint64_t (rest[5]) << 24) | (uint64_t (rest[6]) << 16) | (uint64_t (rest[7]) << 8) | uint64_t (rest[8]));
        if (value >= 0xFEFDFBF7EFDFBF80ULL)
            return false;
        value += 0x0102040810204080ULL;
        off += 9;
    }
    return true;
}
size_t EncodeUint64Id (char* encoded, uint64_t id)
{
    uint8_t* uencoded = (uint8_t*) encoded;
    if (id < 0x80ULL) {
        uencoded[0] = char (id);
        return 1;
    } else if (id < 0x4080ULL) {
        id -= 0x80ULL;
        uencoded[0] = (id >> 8) | 0x80;
        uencoded[1] = id & 0xff;
        return 2;
    } else if (id < 0x204080ULL) {
        id -= 0x4080ULL;
        uencoded[0] = (id >> 16) | 0xc0;
        uencoded[1] = (id >> 8) & 0xff;
        uencoded[2] = id & 0xff;
        return 3;
    } else if (id < 0x10204080ULL) {
        id -= 0x204080ULL;
        uencoded[0] = (id >> 24) | 0xe0;
        uencoded[1] = (id >> 16) & 0xff;
        uencoded[2] = (id >> 8) & 0xff;
        uencoded[3] = id & 0xff;
        return 4;
    } else if (id < 0x0810204080ULL) {
        id -= 0x10204080ULL;
        uencoded[0] = (id >> 32) | 0xf0;
        uencoded[1] = (id >> 24) & 0xff;
        uencoded[2] = (id >> 16) & 0xff;
        uencoded[3] = (id >> 8) & 0xff;
        uencoded[4] = id & 0xff;
        return 5;
    } else if (id < 0x040810204080ULL) {
        id -= 0x0810204080ULL;
        uencoded[0] = (id >> 40) | 0xf8;
        uencoded[1] = (id >> 32) & 0xff;
        uencoded[2] = (id >> 24) & 0xff;
        uencoded[3] = (id >> 16) & 0xff;
        uencoded[4] = (id >> 8) & 0xff;
        uencoded[5] = id & 0xff;
        return 6;
    } else if (id < 0x02040810204080ULL) {
        id -= 0x040810204080ULL;
        uencoded[0] = (id >> 48) | 0xfc;
        uencoded[1] = (id >> 40) & 0xff;
        uencoded[2] = (id >> 32) & 0xff;
        uencoded[3] = (id >> 24) & 0xff;
        uencoded[4] = (id >> 16) & 0xff;
        uencoded[5] = (id >> 8) & 0xff;
        uencoded[6] = id & 0xff;
        return 7;
    } else if (id < 0x0102040810204080ULL) {
        id -= 0x02040810204080ULL;
        uencoded[0] = (id >> 56) | 0xfe;
        uencoded[1] = (id >> 48) & 0xff;
        uencoded[2] = (id >> 40) & 0xff;
        uencoded[3] = (id >> 32) & 0xff;
        uencoded[4] = (id >> 24) & 0xff;
        uencoded[5] = (id >> 16) & 0xff;
        uencoded[6] = (id >> 8) & 0xff;
        uencoded[7] = id & 0xff;
        return 8;
    } else {
        id -= 0x0102040810204080ULL;
        uencoded[0] = 0xff;
        uencoded[1] = (id >> 56) & 0xff;
        uencoded[2] = (id >> 48) & 0xff;
        uencoded[3] = (id >> 40) & 0xff;
        uencoded[4] = (id >> 32) & 0xff;
        uencoded[5] = (id >> 24) & 0xff;
        uencoded[6] = (id >> 16) & 0xff;
        uencoded[7] = (id >> 8) & 0xff;
        uencoded[8] = id & 0xff;
        return 9;
    }
}

} // namespace HCCN::Internal
