// common/Network/PacketParser.cpp
#include "common/Network/PacketParser.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

namespace bookclub::common {

PacketParser::PacketParser() = default;

QByteArray PacketParser::pack(const Message& message) {
    QJsonObject json = message.toJson();
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_12);

    // هدر: ۴ بایت برای طول داده‌های JSON
    quint32 payloadSize = static_cast<quint32>(jsonData.size());
    stream << payloadSize;
    stream.writeRawData(jsonData.data(), jsonData.size());

    return packet;
}

void PacketParser::feed(const QByteArray& data) {
    m_buffer.append(data);
}

bool PacketParser::hasNextPacket() const {
    if (m_buffer.size() < static_cast<int>(sizeof(quint32))) {
        return false;
    }

    // اگر سایز بعدی را نمی‌دانیم، از بافر بخوانیم
    if (m_nextExpectedSize == 0) {
        QDataStream stream(m_buffer);
        stream.setVersion(QDataStream::Qt_5_12);
        stream >> const_cast<quint32&>(m_nextExpectedSize);
    }

    // بررسی اینکه آیا به اندازه‌ی کافی داده داریم یا نه
    return static_cast<quint32>(m_buffer.size()) >= (sizeof(quint32) + m_nextExpectedSize);
}

Message PacketParser::nextPacket() {
    if (!hasNextPacket()) {
        return Message(Command::Invalid, Status::BadRequest);
    }

    // حذف هدر ۴ بایتی از ابتدای بافر
    m_buffer.remove(0, sizeof(quint32));

    // استخراج داده‌های JSON
    QByteArray jsonData = m_buffer.left(m_nextExpectedSize);
    m_buffer.remove(0, m_nextExpectedSize);
    m_nextExpectedSize = 0;

    // تبدیل JSON به Message
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "PacketParser: JSON Parse Error:" << parseError.errorString();
        return Message(Command::Invalid, Status::BadRequest);
    }

    if (!doc.isObject()) {
        return Message(Command::Invalid, Status::BadRequest);
    }

    return Message::fromJson(doc.object());
}

void PacketParser::clear() {
    m_buffer.clear();
    m_nextExpectedSize = 0;
}

} // namespace bookclub::common
