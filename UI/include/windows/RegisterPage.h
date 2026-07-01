#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QResizeEvent>

class RegisterPage : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterPage(QWidget *parent = nullptr);

signals:
    void navigateToLogin();
    void registerSuccess(const QString &username);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onRegisterClicked();

private:
    void setupUi();
    void setError(const QString &message);
    void clearError();
    static bool isValidEmail(const QString &email);
    static void stylePlaceholder(QLineEdit *edit);

    QWidget     *m_card = nullptr;
    QLineEdit   *m_usernameEdit;
    QLineEdit   *m_emailEdit;
    QLineEdit   *m_passwordEdit;
    QLineEdit   *m_confirmEdit;
    QComboBox   *m_questionCombo;
    QLineEdit   *m_answerEdit;
    QLabel      *m_errorLabel;
    QPushButton *m_registerButton;
};
