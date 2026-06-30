// common/Network/Message.cpp
#include "common/Network/Message.h"

namespace bookclub::common {

Message::Message() : m_requestId(QUuid::createUuid().toString()) {}

Message::Message(Command cmd) : m_command(cmd), m_requestId(QUuid::createUuid().toString()) {}

Message::Message(Command cmd, Status status)
    : m_command(cmd), m_status(status), m_requestId(QUuid::createUuid().toString()) {}

Message::Message(Command cmd, const QJsonObject& payload)
    : m_command(cmd), m_payload(payload), m_requestId(QUuid::createUuid().toString()) {}

Message::Message(Command cmd, Status status, const QJsonObject& payload)
    : m_command(cmd), m_status(status), m_payload(payload), m_requestId(QUuid::createUuid().toString()) {}

// ---- Getters ----
Command Message::command() const { return m_command; }
Status Message::status() const { return m_status; }
QJsonObject Message::payload() const { return m_payload; }
QString Message::requestId() const { return m_requestId; }
QString Message::errorMessage() const { return m_errorMessage; }

// ---- Setters ----
void Message::setCommand(Command cmd) { m_command = cmd; }
void Message::setStatus(Status status) { m_status = status; }
void Message::setPayload(const QJsonObject& payload) { m_payload = payload; }
void Message::setErrorMessage(const QString& error) { m_errorMessage = error; }

// ---- Serialization ----
QJsonObject Message::toJson() const {
    QJsonObject root;
    root["requestId"] = m_requestId;
    root["command"] = static_cast<quint16>(m_command);
    root["status"] = static_cast<quint16>(m_status);
    root["payload"] = m_payload;
    if (!m_errorMessage.isEmpty()) {
        root["errorMessage"] = m_errorMessage;
    }
    return root;
}

Message Message::fromJson(const QJsonObject& json) {
    Command cmd = Command::Invalid;
    Status status = Status::Success;

    if (json.contains("command") && json["command"].isDouble()) {
        cmd = static_cast<Command>(json["command"].toInteger());
    }
    if (json.contains("status") && json["status"].isDouble()) {
        status = static_cast<Status>(json["status"].toInteger());
    }

    Message msg(cmd, status);
    if (json.contains("requestId") && json["requestId"].isString()) {
        msg.m_requestId = json["requestId"].toString();
    }
    if (json.contains("payload") && json["payload"].isObject()) {
        msg.m_payload = json["payload"].toObject();
    }
    if (json.contains("errorMessage") && json["errorMessage"].isString()) {
        msg.m_errorMessage = json["errorMessage"].toString();
    }
    return msg;
}

bool Message::isValid() const {
    return m_command != Command::Invalid;
}

bool Message::isSuccess() const {
    return m_status == Status::Success;
}

} // namespace bookclub::common
