#include "windows/LoginPage.h"
#include "auth/AuthManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QPalette>

LoginPage::LoginPage(QWidget *parent)
    : QWidget(parent)
{
    // Transparent — the MainWindow gradient shows through
    setAutoFillBackground(false);
    setupUi();
}

void LoginPage::stylePlaceholder(QLineEdit *edit)
{
    QPalette pal = edit->palette();
    pal.setColor(QPalette::PlaceholderText, QColor("#90C4A2"));
    edit->setPalette(pal);
}

void LoginPage::setupUi()
{
    // ── Scroll area wraps everything so content survives small windows ────
    auto *scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setAutoFillBackground(false);
    scroll->viewport()->setAutoFillBackground(false);

    // ── Inner container — transparent, centres the card ──────────────────
    auto *inner = new QWidget();
    inner->setAutoFillBackground(false);

    auto *innerLayout = new QVBoxLayout(inner);
    innerLayout->setContentsMargins(24, 28, 24, 28);
    innerLayout->setAlignment(Qt::AlignCenter);

    // ── Card ─────────────────────────────────────────────────────────────
    m_card = new QWidget(inner);
    m_card->setObjectName("card");
    m_card->setFixedWidth(420);  // initial; resizeEvent keeps it fluid

    auto *shadow = new QGraphicsDropShadowEffect(m_card);
    shadow->setBlurRadius(55);
    shadow->setColor(QColor(40, 25, 10, 38));
    shadow->setOffset(0, 14);
    m_card->setGraphicsEffect(shadow);

    // ── Card content ──────────────────────────────────────────────────────
    auto *cl = new QVBoxLayout(m_card);
    cl->setContentsMargins(44, 50, 44, 50);
    cl->setSpacing(0);

    auto *logoLabel = new QLabel("\xF0\x9F\x93\x9A", m_card);
    logoLabel->setObjectName("logoLabel");
    logoLabel->setAlignment(Qt::AlignCenter);
    cl->addWidget(logoLabel);
    cl->addSpacing(10);

    auto *appNameLabel = new QLabel("BOOK CLUB", m_card);
    appNameLabel->setObjectName("appNameLabel");
    appNameLabel->setAlignment(Qt::AlignCenter);
    cl->addWidget(appNameLabel);
    cl->addSpacing(22);

    auto *titleLabel = new QLabel("Welcome back", m_card);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    cl->addWidget(titleLabel);
    cl->addSpacing(6);

    auto *subtitleLabel = new QLabel("Sign in to continue reading", m_card);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    cl->addWidget(subtitleLabel);
    cl->addSpacing(38);

    m_usernameEdit = new QLineEdit(m_card);
    m_usernameEdit->setPlaceholderText("Username");
    stylePlaceholder(m_usernameEdit);
    cl->addWidget(m_usernameEdit);
    cl->addSpacing(14);

    m_passwordEdit = new QLineEdit(m_card);
    m_passwordEdit->setPlaceholderText("Password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    stylePlaceholder(m_passwordEdit);
    cl->addWidget(m_passwordEdit);
    cl->addSpacing(14);

    m_errorLabel = new QLabel(m_card);
    m_errorLabel->setObjectName("errorLabel");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    cl->addWidget(m_errorLabel);

    cl->addSpacing(22);

    m_loginButton = new QPushButton("Sign In", m_card);
    m_loginButton->setObjectName("primaryButton");
    m_loginButton->setCursor(Qt::PointingHandCursor);
    cl->addWidget(m_loginButton);
    cl->addSpacing(14);

    auto *forgotBtn = new QPushButton("Forgot your password?", m_card);
    forgotBtn->setObjectName("linkButton");
    forgotBtn->setCursor(Qt::PointingHandCursor);
    cl->addWidget(forgotBtn, 0, Qt::AlignCenter);
    cl->addSpacing(30);

    auto *divider = new QFrame(m_card);
    divider->setObjectName("divider");
    divider->setFrameShape(QFrame::HLine);
    cl->addWidget(divider);
    cl->addSpacing(24);

    auto *signupRow = new QHBoxLayout();
    signupRow->setAlignment(Qt::AlignCenter);
    signupRow->setSpacing(6);
    auto *signupLabel = new QLabel("Don't have an account?", m_card);
    signupLabel->setObjectName("smallLabel");
    auto *signupBtn = new QPushButton("Create one", m_card);
    signupBtn->setObjectName("inlineLinkButton");
    signupBtn->setCursor(Qt::PointingHandCursor);
    signupRow->addWidget(signupLabel);
    signupRow->addWidget(signupBtn);
    cl->addLayout(signupRow);

    // ── Wire up ───────────────────────────────────────────────────────────
    connect(m_loginButton,  &QPushButton::clicked,     this, &LoginPage::onLoginClicked);
    connect(forgotBtn,      &QPushButton::clicked,     this, &LoginPage::navigateToReset);
    connect(signupBtn,      &QPushButton::clicked,     this, &LoginPage::navigateToRegister);
    connect(m_usernameEdit, &QLineEdit::returnPressed, this, &LoginPage::onLoginClicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginPage::onLoginClicked);

    innerLayout->addWidget(m_card);
    scroll->setWidget(inner);

    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scroll);
}

// ---------------------------------------------------------------------------
// Keep the card width proportional to the window — clamped to [320, 460].
// ---------------------------------------------------------------------------
void LoginPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_card) {
        constexpr int hMargin = 56;
        constexpr int minW    = 320;
        constexpr int maxW    = 460;
        m_card->setFixedWidth(qBound(minW, width() - hMargin, maxW));
    }
}

void LoginPage::onLoginClicked()
{
    clearError();

    const QString username = m_usernameEdit->text().trimmed();
    const QString password = m_passwordEdit->text();

    if (username.isEmpty()) { setError("Please enter your username."); return; }
    if (password.isEmpty()) { setError("Please enter your password."); return; }

    if (AuthManager::instance().loginUser(username, password)) {
        m_passwordEdit->clear();
        emit loginSuccess(username);
    } else {
        setError(AuthManager::instance().lastError());
    }
}

void LoginPage::setError(const QString &message)
{
    m_errorLabel->setText(message);
    m_errorLabel->show();
}

void LoginPage::clearError()
{
    m_errorLabel->hide();
    m_errorLabel->clear();
}
