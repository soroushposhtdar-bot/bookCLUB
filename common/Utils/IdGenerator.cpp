// common/Utils/IdGenerator.cpp
#include "common/Utils/IdGenerator.h"

#include <QUuid>
#include <QRandomGenerator>

namespace bookclub::common {

QString IdGenerator::generateUuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString IdGenerator::generateShortId(int length)
{
    static const QString alphabet = "abcdefghijklmnopqrstuvwxyz0123456789";
    QString result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        int index = QRandomGenerator::global()->bounded(alphabet.length());
        result.append(alphabet.at(index));
    }
    return result;
}

} // namespace bookclub::common
