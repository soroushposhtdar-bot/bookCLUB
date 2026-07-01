#include "windows/RegisterPage.h"
#include "auth/AuthManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QRegularExpression>
#include <QGraphicsDropShadowEffect>
#include <QPalette>

RegisterPage::RegisterPage(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(false);
    setupUi();
}

void RegisterPage::stylePlaceholder(QLineEdit *edit)
{
    QPalette pal = edit->palette();
    pal.setColor(QPalette::PlaceholderText, QColor("#90C4A2"));
    edit->setPalette(pal);
}

bool RegisterPage::isValidEmail(const QString &email)
{
    static const QRegularExpression re(
        R"(^[a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}$)");
    return re.match(email).hasMatch();
}

void RegisterPage::setupUi()
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
    innerLayout->setContentsMargins(24, 22, 24, 22);
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

    // ── Card content ──────────────────────────────────────────────────────
    auto *cl = new QVBoxLayout(m_card);
    cl->setContentsMargins(44, 42, 44, 42);
    cl->setSpacing(0);

    auto *logoLabel = new QLabel("\xF0\x9F\x93\x9A", m_card);
    logoLabel->setObjectName("logoLabel");
    logoLabel->setAlignment(Qt::AlignCenter);
    cl->addWidget(logoLabel);
    cl->addSpacing(8);

    auto *appNameLabel = new QLabel("BOOK CLUB", m_card);
    appNameLabel->setObjectName("appNameLabel");
    appNameLabel->setAlignment(Qt::AlignCenter);
    cl->addWidget(appNameLabel);
    cl->addSpacing(18);

    auto *titleLabel = new QLabel("Create account", m_card);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    cl->addWidget(titleLabel);
    cl->addSpacing(5);

    auto *subtitleLabel = new QLabel("Join the reading community", m_card);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    cl->addWidget(subtitleLabel);
    cl->addSpacing(28);

    m_usernameEdit = new QLineEdit(m_card);
    m_usernameEdit->setPlaceholderText("Username");
    stylePlaceholder(m_usernameEdit);
    cl->addWidget(m_usernameEdit);
    cl->addSpacing(12);

    m_emailEdit = new QLineEdit(m_card);
    m_emailEdit->setPlaceholderText("Email address");
    stylePlaceholder(m_emailEdit);
    cl->addWidget(m_emailEdit);
    cl->addSpacing(12);

    m_passwordEdit = new QLineEdit(m_card);
    m_passwordEdit->setPlaceholderText("Password  (min. 6 characters)");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    stylePlaceholder(m_passwordEdit);
    cl->addWidget(m_passwordEdit);
    cl->addSpacing(12);

    m_confirmEdit = new QLineEdit(m_card);
    m_confirmEdit->setPlaceholderText("Confirm password");
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    stylePlaceholder(m_confirmEdit);
    cl->addWidget(m_confirmEdit);
    cl->addSpacing(18);

    // ── Security question ─────────────────────────────────────────────────
    auto *secLabel = new QLabel("Security Question", m_card);
    secLabel->setObjectName("fieldLabel");
    cl->addWidget(secLabel);
    cl->addSpacing(7);

    m_questionCombo = new QComboBox(m_card);
    m_questionCombo->setCursor(Qt::PointingHandCursor);
    m_questionCombo->addItem("Select a security question...");
    m_questionCombo->addItem("Is Soroush the greatest person of all time?");
    m_questionCombo->addItem("What was the name of your first school?");
    m_questionCombo->addItem("Who was your childhood best friend?");
    m_questionCombo->addItem("What is your favorite teacher's last name?");
    m_questionCombo->addItem("What city were you born in?");
    m_questionCombo->addItem("What is your favorite movie?");
    cl->addWidget(m_questionCombo);
    cl->addSpacing(12);

    m_answerEdit = new QLineEdit(m_card);
    m_answerEdit->setPlaceholderText("Your answer");
    stylePlaceholder(m_answerEdit);
    cl->addWidget(m_answerEdit);
    cl->addSpacing(12);

    // ── Error label ───────────────────────────────────────────────────────
    m_errorLabel = new QLabel(m_card);
    m_errorLabel->setObjectName("errorLabel");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    cl->addWidget(m_errorLabel);

    cl->addSpacing(18);

    m_registerButton = new QPushButton("Create Account", m_card);
    m_registerButton->setObjectName("primaryButton");
    m_registerButton->setCursor(Qt::PointingHandCursor);
    cl->addWidget(m_registerButton);
    cl->addSpacing(22);

    auto *divider = new QFrame(m_card);
    divider->setObjectName("divider");
    divider->setFrameShape(QFrame::HLine);
    cl->addWidget(divider);
    cl->addSpacing(20);

    auto *signinRow = new QHBoxLayout();
    signinRow->setAlignment(Qt::AlignCenter);
    signinRow->setSpacing(6);
    auto *signinLabel = new QLabel("Already have an account?", m_card);
    signinLabel->setObjectName("smallLabel");
    auto *signinBtn = new QPushButton("Sign In", m_card);
    signinBtn->setObjectName("inlineLinkButton");
    signinBtn->setCursor(Qt::PointingHandCursor);
    signinRow->addWidget(signinLabel);
    signinRow->addWidget(signinBtn);
    cl->addLayout(signinRow);

    // ── Wire up ───────────────────────────────────────────────────────────
    connect(m_registerButton, &QPushButton::clicked,     this, &RegisterPage::onRegisterClicked);
    connect(signinBtn,        &QPushButton::clicked,     this, &RegisterPage::navigateToLogin);
    connect(m_answerEdit,     &QLineEdit::returnPressed, this, &RegisterPage::onRegisterClicked);

    innerLayout->addWidget(m_card);
    scroll->setWidget(inner);

    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scroll);
}

void RegisterPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_card) {
        constexpr int hMargin = 56;
        constexpr int minW    = 320;
        constexpr int maxW    = 460;
        m_card->setFixedWidth(qBound(minW, width() - hMargin, maxW));
    }
}

void RegisterPage::onRegisterClicked()
{
    clearError();

    const QString username    = m_usernameEdit->text().trimmed();
    const QString email       = m_emailEdit->text().trimmed();
    const QString password    = m_passwordEdit->text();
    const QString confirm     = m_confirmEdit->text();
    const int     questionIdx = m_questionCombo->currentIndex();
    const QString answer      = m_answerEdit->text().trimmed();

    if (username.isEmpty())     { setError("Please choose a username.");                    return; }
    if (username.length() < 3)  { setError("Username must be at least 3 characters.");      return; }
    if (email.isEmpty())        { setError("Please enter your email address.");             return; }
    if (!isValidEmail(email))   { setError("Please enter a valid email address.");          return; }
    if (password.isEmpty())     { setError("Please create a password.");                    return; }
    if (password.length() < 6)  { setError("Password must be at least 6 characters.");     return; }
    if (password != confirm)    { setError("Passwords do not match.");                      return; }
    if (questionIdx == 0)       { setError("Please select a security question.");           return; }
    if (answer.isEmpty())       { setError("Please enter an answer to your security question."); return; }

    const QString question = m_questionCombo->currentText();

    if (AuthManager::instance().registerUser(username, email, password, question, answer)) {
        m_usernameEdit->clear();
        m_emailEdit->clear();
        m_passwordEdit->clear();
        m_confirmEdit->clear();
        m_questionCombo->setCurrentIndex(0);
        m_answerEdit->clear();
        emit registerSuccess(username);
    } else {
        setError(AuthManager::instance().lastError());
    }
}

void RegisterPage::setError(const QString &message)
{
    m_errorLabel->setText(message);
    m_errorLabel->show();
}

void RegisterPage::clearError()
{
    m_errorLabel->hide();
    m_errorLabel->clear();
}
