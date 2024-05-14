#pragma once

#include <optional>
#include <vector>
#include <QHostAddress>
#include <QNetworkDatagram>

namespace HCCN::Internal {

bool ParseUint64Id (const std::vector<char>& data, size_t& off, uint64_t& value);
size_t EncodeUint64Id (char* encoded, uint64_t id);

} // namespace HCCN::Internal
