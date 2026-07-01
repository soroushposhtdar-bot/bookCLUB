#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QResizeEvent>

class LoginPage : public QWidget
{
    Q_OBJECT

public:
    explicit LoginPage(QWidget *parent = nullptr);

signals:
    void navigateToRegister();
    void navigateToReset();
    void loginSuccess(const QString &username);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onLoginClicked();

private:
    void setupUi();
    void setError(const QString &message);
    void clearError();
    static void stylePlaceholder(QLineEdit *edit);

    QWidget     *m_card = nullptr;
    QLineEdit   *m_usernameEdit;
    QLineEdit   *m_passwordEdit;
    QLabel      *m_errorLabel;
    QPushButton *m_loginButton;
};
