// common/Network/Message.h
#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QString>
#include <QUuid>

#include "common/Network/Protocol.h"

namespace bookclub::common {

class Message {
public:
    Message();
    explicit Message(Command cmd);
    Message(Command cmd, Status status);
    Message(Command cmd, const QJsonObject& payload);
    Message(Command cmd, Status status, const QJsonObject& payload);

    // --- Getters ---
    Command command() const;
    Status status() const;
    QJsonObject payload() const;
    QString requestId() const;
    QString errorMessage() const;

    // --- Setters ---
    void setCommand(Command cmd);
    void setStatus(Status status);
    void setPayload(const QJsonObject& payload);
    void setErrorMessage(const QString& error);

    // --- Serialization (JSON) ---
    QJsonObject toJson() const;
    static Message fromJson(const QJsonObject& json);

    // --- Utility ---
    bool isValid() const;
    bool isSuccess() const;

private:
    Command m_command = Command::Invalid;
    Status m_status = Status::Success;
    QJsonObject m_payload;
    QString m_requestId;
    QString m_errorMessage;
};

} // namespace bookclub::common
