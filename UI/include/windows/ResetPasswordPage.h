#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QResizeEvent>

// Two-step password reset:
//   Step 0 — user enters their email; the app finds and shows their security question.
//   Step 1 — user answers the question, then chooses a new password.
class ResetPasswordPage : public QWidget
{
    Q_OBJECT

public:
    explicit ResetPasswordPage(QWidget *parent = nullptr);

signals:
    void navigateToLogin();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onLookupQuestion();
    void onResetPassword();
    void onBackToStep0();

private:
    void setupUi();

    void setErrorStep0(const QString &msg);
    void clearErrorStep0();
    void setErrorStep1(const QString &msg);
    void clearErrorStep1();
    static void stylePlaceholder(QLineEdit *edit);

    QWidget        *m_card  = nullptr;
    QStackedWidget *m_steps = nullptr;

    // Step 0 widgets
    QLineEdit   *m_emailEdit;
    QLabel      *m_errorStep0;
    QPushButton *m_continueBtn;

    // Step 1 widgets
    QLabel      *m_questionDisplayLabel;
    QLineEdit   *m_answerEdit;
    QLineEdit   *m_newPassEdit;
    QLineEdit   *m_confirmPassEdit;
    QLabel      *m_errorStep1;
    QPushButton *m_resetBtn;

    // State
    QString m_resetEmail;
};
