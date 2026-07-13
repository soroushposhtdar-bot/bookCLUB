#pragma once

#include <QMainWindow>

namespace bookclub::client {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

signals:
    void loginRequested();
    void logoutRequested();

private:
    void setupUi();
};

} // namespace bookclub::client
