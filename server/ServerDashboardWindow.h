#pragma once

#include <QMainWindow>

namespace bookclub::server {

class ServerDashboardWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit ServerDashboardWindow(QWidget* parent = nullptr);
    ~ServerDashboardWindow() override = default;

signals:
    void startRequested();
    void stopRequested();

private:
    void setupUi();
};

} // namespace bookclub::server
