// common/Network/PacketParser.h
#pragma once

#include <QByteArray>
#include <QQueue>
#include <QDataStream>

#include "common/Network/Message.h"

namespace bookclub::common {

class PacketParser {
public:
    PacketParser();

    // بسته‌بندی پیام برای ارسال (اضافه کردن هدر ۴ بایتی طول)
    static QByteArray pack(const Message& message);

    // خوراک دادن داده‌های خام دریافتی از سوکت
    void feed(const QByteArray& data);

    // بررسی وجود یک بسته کامل در بافر
    bool hasNextPacket() const;

    // دریافت بسته‌ی کامل بعدی (اگر موجود باشد)
    Message nextPacket();

    // پاک کردن بافر
    void clear();

private:
    QByteArray m_buffer;
    quint32 m_nextExpectedSize = 0;
};

} // namespace bookclub::common
