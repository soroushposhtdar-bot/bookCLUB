#pragma once

#include <QWidget>

namespace bookclub::client {

class LoginWindow : public QWidget {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow() override = default;

signals:
    void loginSubmitted();
    void registerSubmitted();
    void resetPasswordSubmitted();

private:
    void setupUi();
};

} // namespace bookclub::client
