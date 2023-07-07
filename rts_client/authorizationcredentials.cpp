#include "authorizationcredentials.h"

AuthorizationCredentials::AuthorizationCredentials (const QString& host, quint16 port, const QString& login, const QString& password)
    : host (host)
    , port (port)
    , login (login)
    , password (password)
{
}
