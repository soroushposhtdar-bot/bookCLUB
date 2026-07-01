#pragma once

#include <QWidget>
#include <QStackedWidget>

class LoginPage;
class RegisterPage;
class ResetPasswordPage;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void showLogin();
    void showRegister();
    void showReset();
    void onLoginSuccess(const QString &username);
    void onRegisterSuccess(const QString &username);

private:
    void applyAppStyle();

    QStackedWidget    *m_stack;
    LoginPage         *m_loginPage;
    RegisterPage      *m_registerPage;
    ResetPasswordPage *m_resetPage;
};
