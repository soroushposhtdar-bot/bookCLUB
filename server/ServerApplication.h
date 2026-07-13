#pragma once

#include <QObject>

namespace bookclub::server {

class ServerCore;

class ServerApplication : public QObject {
    Q_OBJECT
public:
    explicit ServerApplication(QObject* parent = nullptr);
    ~ServerApplication() override = default;

    bool start();
    void stop();
    bool isRunning() const;

private:
    ServerCore* m_core = nullptr;
};

} // namespace bookclub::server
