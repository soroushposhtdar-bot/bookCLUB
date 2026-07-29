#pragma once

#include <QString>

namespace bookclub::common {

class ServerHealth {
public:
    QString state;
    double cpuUsage = 0.0;
    double ramUsage = 0.0;
    int onlineClients = 0;
    bool overloaded = false;
};

} // namespace bookclub::common
