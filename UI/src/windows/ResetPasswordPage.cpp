#include "windows/ResetPasswordPage.h"
#include "auth/AuthManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QPalette>

ResetPasswordPage::ResetPasswordPage(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(false);
    setupUi();
}

void ResetPasswordPage::stylePlaceholder(QLineEdit *edit)
{
    QPalette pal = edit->palette();
    pal.setColor(QPalette::PlaceholderText, QColor("#90C4A2"));
    edit->setPalette(pal);
}

// ---------------------------------------------------------------------------
void ResetPasswordPage::setupUi()
{
    // ── Scroll area ───────────────────────────────────────────────────────
    auto *scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setAutoFillBackground(false);
    scroll->viewport()->setAutoFillBackground(false);

    auto *inner = new QWidget();
    inner->setAutoFillBackground(false);

    auto *innerLayout = new QVBoxLayout(inner);
    innerLayout->setContentsMargins(24, 28, 24, 28);
    innerLayout->setAlignment(Qt::AlignCenter);

    // ── Card ──────────────────────────────────────────────────────────────
    m_card = new QWidget(inner);
    m_card->setObjectName("card");
    m_card->setFixedWidth(420);

    auto *shadow = new QGraphicsDropShadowEffect(m_card);
    shadow->setBlurRadius(55);
    shadow->setColor(QColor(10, 50, 25, 50));
    shadow->setOffset(0, 14);
    m_card->setGraphicsEffect(shadow);

    auto *cardLayout = new QVBoxLayout(m_card);
    cardLayout->setContentsMargins(44, 48, 44, 48);
    cardLayout->setSpacing(0);

    // Logo + app name (always visible regardless of step)
    auto *logoLabel = new QLabel("\xF0\x9F\x93\x9A", m_card);
    logoLabel->setObjectName("logoLabel");
    logoLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(logoLabel);
    cardLayout->addSpacing(10);

    auto *appNameLabel = new QLabel("BOOK CLUB", m_card);
    appNameLabel->setObjectName("appNameLabel");
    appNameLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(appNameLabel);
    cardLayout->addSpacing(26);

    // ── Step stack ────────────────────────────────────────────────────────
    m_steps = new QStackedWidget(m_card);
    m_steps->setAutoFillBackground(false);
    m_steps->setStyleSheet("QStackedWidget { background: transparent; }");
    cardLayout->addWidget(m_steps);

    // ==================================================================
    // Step 0 — Enter email
    // ==================================================================
    auto *step0 = new QWidget();
    step0->setAutoFillBackground(false);
    step0->setStyleSheet("QWidget { background: transparent; }");

    auto *s0 = new QVBoxLayout(step0);
    s0->setContentsMargins(0, 0, 0, 0);
    s0->setSpacing(0);

    auto *stepLabelA = new QLabel("STEP 1 OF 2", step0);
    stepLabelA->setObjectName("stepLabel");
    stepLabelA->setAlignment(Qt::AlignCenter);
    s0->addWidget(stepLabelA);
    s0->addSpacing(10);

    auto *titleA = new QLabel("Reset password", step0);
    titleA->setObjectName("titleLabel");
    titleA->setAlignment(Qt::AlignCenter);
    s0->addWidget(titleA);
    s0->addSpacing(6);

    auto *subtitleA = new QLabel("Enter your email to verify your identity", step0);
    subtitleA->setObjectName("subtitleLabel");
    subtitleA->setAlignment(Qt::AlignCenter);
    subtitleA->setWordWrap(true);
    s0->addWidget(subtitleA);
    s0->addSpacing(30);

    m_emailEdit = new QLineEdit(step0);
    m_emailEdit->setPlaceholderText("Email address");
    stylePlaceholder(m_emailEdit);
    s0->addWidget(m_emailEdit);
    s0->addSpacing(14);

    m_errorStep0 = new QLabel(step0);
    m_errorStep0->setObjectName("errorLabel");
    m_errorStep0->setAlignment(Qt::AlignCenter);
    m_errorStep0->setWordWrap(true);
    m_errorStep0->hide();
    s0->addWidget(m_errorStep0);

    s0->addSpacing(20);

    m_continueBtn = new QPushButton("Continue", step0);
    m_continueBtn->setObjectName("primaryButton");
    m_continueBtn->setCursor(Qt::PointingHandCursor);
    s0->addWidget(m_continueBtn);
    s0->addSpacing(22);

    auto *backRow0 = new QHBoxLayout();
    backRow0->setAlignment(Qt::AlignCenter);
    backRow0->setSpacing(6);
    auto *rememberLabel = new QLabel("Remember your password?", step0);
    rememberLabel->setObjectName("smallLabel");
    auto *backLoginBtn0 = new QPushButton("Sign In", step0);
    backLoginBtn0->setObjectName("inlineLinkButton");
    backLoginBtn0->setCursor(Qt::PointingHandCursor);
    backRow0->addWidget(rememberLabel);
    backRow0->addWidget(backLoginBtn0);
    s0->addLayout(backRow0);

    m_steps->addWidget(step0); // index 0

    // ==================================================================
    // Step 1 — Answer security question + new password
    // ==================================================================
    auto *step1 = new QWidget();
    step1->setAutoFillBackground(false);
    step1->setStyleSheet("QWidget { background: transparent; }");

    auto *s1 = new QVBoxLayout(step1);
    s1->setContentsMargins(0, 0, 0, 0);
    s1->setSpacing(0);

    auto *stepLabelB = new QLabel("STEP 2 OF 2", step1);
    stepLabelB->setObjectName("stepLabel");
    stepLabelB->setAlignment(Qt::AlignCenter);
    s1->addWidget(stepLabelB);
    s1->addSpacing(10);

    auto *titleB = new QLabel("Security Question", step1);
    titleB->setObjectName("titleLabel");
    titleB->setAlignment(Qt::AlignCenter);
    s1->addWidget(titleB);
    s1->addSpacing(6);

    auto *subtitleB = new QLabel("Answer your question, then choose a new password", step1);
    subtitleB->setObjectName("subtitleLabel");
    subtitleB->setAlignment(Qt::AlignCenter);
    subtitleB->setWordWrap(true);
    s1->addWidget(subtitleB);
    s1->addSpacing(22);

    // Displays the user's registered security question
    m_questionDisplayLabel = new QLabel(step1);
    m_questionDisplayLabel->setObjectName("questionDisplayLabel");
    m_questionDisplayLabel->setWordWrap(true);
    m_questionDisplayLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    s1->addWidget(m_questionDisplayLabel);
    s1->addSpacing(14);

    m_answerEdit = new QLineEdit(step1);
    m_answerEdit->setPlaceholderText("Your answer");
    stylePlaceholder(m_answerEdit);
    s1->addWidget(m_answerEdit);
    s1->addSpacing(12);

    m_newPassEdit = new QLineEdit(step1);
    m_newPassEdit->setPlaceholderText("New password  (min. 6 characters)");
    m_newPassEdit->setEchoMode(QLineEdit::Password);
    stylePlaceholder(m_newPassEdit);
    s1->addWidget(m_newPassEdit);
    s1->addSpacing(12);

    m_confirmPassEdit = new QLineEdit(step1);
    m_confirmPassEdit->setPlaceholderText("Confirm new password");
    m_confirmPassEdit->setEchoMode(QLineEdit::Password);
    stylePlaceholder(m_confirmPassEdit);
    s1->addWidget(m_confirmPassEdit);
    s1->addSpacing(12);

    m_errorStep1 = new QLabel(step1);
    m_errorStep1->setObjectName("errorLabel");
    m_errorStep1->setAlignment(Qt::AlignCenter);
    m_errorStep1->setWordWrap(true);
    m_errorStep1->hide();
    s1->addWidget(m_errorStep1);

    s1->addSpacing(18);

    m_resetBtn = new QPushButton("Reset Password", step1);
    m_resetBtn->setObjectName("primaryButton");
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    s1->addWidget(m_resetBtn);
    s1->addSpacing(16);

    auto *backBtn1 = new QPushButton("\u2190 Back", step1);
    backBtn1->setObjectName("linkButton");
    backBtn1->setCursor(Qt::PointingHandCursor);
    s1->addWidget(backBtn1, 0, Qt::AlignCenter);

    m_steps->addWidget(step1); // index 1

    // ── Wire up ───────────────────────────────────────────────────────────
    connect(m_continueBtn,     &QPushButton::clicked,        this, &ResetPasswordPage::onLookupQuestion);
    connect(m_emailEdit,       &QLineEdit::returnPressed,    this, &ResetPasswordPage::onLookupQuestion);

    connect(m_resetBtn,        &QPushButton::clicked,        this, &ResetPasswordPage::onResetPassword);
    connect(m_confirmPassEdit, &QLineEdit::returnPressed,    this, &ResetPasswordPage::onResetPassword);

    connect(backBtn1,          &QPushButton::clicked,        this, &ResetPasswordPage::onBackToStep0);
    connect(backLoginBtn0,     &QPushButton::clicked,        this, &ResetPasswordPage::navigateToLogin);

    innerLayout->addWidget(m_card);
    scroll->setWidget(inner);

    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scroll);
}

// ---------------------------------------------------------------------------
void ResetPasswordPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_card) {
        constexpr int hMargin = 56;
        constexpr int minW    = 320;
        constexpr int maxW    = 460;
        m_card->setFixedWidth(qBound(minW, width() - hMargin, maxW));
    }
}

// ---------------------------------------------------------------------------
void ResetPasswordPage::onLookupQuestion()
{
    clearErrorStep0();

    const QString email = m_emailEdit->text().trimmed();
    if (email.isEmpty()) {
        setErrorStep0("Please enter your email address.");
        return;
    }
    if (!AuthManager::instance().emailExists(email)) {
        setErrorStep0("No account is registered with that email.");
        return;
    }

    const QString question = AuthManager::instance().getSecurityQuestion(email);
    if (question.isEmpty()) {
        setErrorStep0("Could not retrieve security question for this account.");
        return;
    }

    m_resetEmail = email;
    m_questionDisplayLabel->setText(question);

    m_answerEdit->clear();
    m_newPassEdit->clear();
    m_confirmPassEdit->clear();
    clearErrorStep1();
    m_steps->setCurrentIndex(1);
}

// ---------------------------------------------------------------------------
void ResetPasswordPage::onResetPassword()
{
    clearErrorStep1();

    const QString answer  = m_answerEdit->text().trimmed();
    const QString newPass = m_newPassEdit->text();
    const QString confirm = m_confirmPassEdit->text();

    if (answer.isEmpty())   { setErrorStep1("Please enter your answer.");               return; }
    if (newPass.isEmpty())  { setErrorStep1("Please enter a new password.");             return; }
    if (newPass.length() < 6) { setErrorStep1("Password must be at least 6 characters."); return; }
    if (newPass != confirm) { setErrorStep1("Passwords do not match.");                  return; }

    if (!AuthManager::instance().verifySecurityAnswer(m_resetEmail, answer)) {
        setErrorStep1(AuthManager::instance().lastError());
        return;
    }

    if (AuthManager::instance().resetPassword(m_resetEmail, newPass)) {
        m_resetEmail.clear();
        m_emailEdit->clear();
        m_answerEdit->clear();
        m_newPassEdit->clear();
        m_confirmPassEdit->clear();
        m_questionDisplayLabel->clear();
        m_steps->setCurrentIndex(0);

        QMessageBox ok(this);
        ok.setWindowTitle("Password updated");
        ok.setIcon(QMessageBox::NoIcon);
        ok.setText(
            "<b style='font-size:15px'>Password reset!</b><br><br>"
            "Your password has been updated.<br>"
            "Sign in with your new credentials."
        );
        ok.setStandardButtons(QMessageBox::Ok);
        ok.exec();

        emit navigateToLogin();
    } else {
        setErrorStep1(AuthManager::instance().lastError());
    }
}

// ---------------------------------------------------------------------------
void ResetPasswordPage::onBackToStep0()
{
    clearErrorStep1();
    m_steps->setCurrentIndex(0);
}

void ResetPasswordPage::setErrorStep0(const QString &msg) { m_errorStep0->setText(msg); m_errorStep0->show(); }
void ResetPasswordPage::clearErrorStep0()                 { m_errorStep0->hide(); m_errorStep0->clear(); }
void ResetPasswordPage::setErrorStep1(const QString &msg) { m_errorStep1->setText(msg); m_errorStep1->show(); }
void ResetPasswordPage::clearErrorStep1()                 { m_errorStep1->hide(); m_errorStep1->clear(); }
