#pragma once

#include <QString>

class AuthorizationCredentials
{
public:
    AuthorizationCredentials (const QString& host, quint16 port, const QString& login, const QString& password);

public:
    QString host;
    quint16 port;
    QString login;
    QString password;
};
