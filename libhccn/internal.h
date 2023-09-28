#pragma once

#include <optional>
#include <QHostAddress>
#include <QByteArray>
#include <QNetworkDatagram>

namespace HCCN::Internal {

bool ParseUint64Id (const QByteArray& data, qsizetype& off, quint64& value);
size_t EncodeUint64Id (char* encoded, quint64 id);

} // namespace HCCN::Internal
