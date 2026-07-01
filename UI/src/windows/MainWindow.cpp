#include "windows/MainWindow.h"
#include "windows/LoginPage.h"
#include "windows/RegisterPage.h"
#include "windows/ResetPasswordPage.h"

#include <QVBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QFile>
#include <QIODevice>

static QString loadAppStyle()
{
    QFile file(":/assets/qss/login.qss");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}


MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("Book Club");
    setMinimumSize(420, 640);
    resize(520, 760);

    applyAppStyle();

    m_stack        = new QStackedWidget(this);
    m_loginPage    = new LoginPage(this);
    m_registerPage = new RegisterPage(this);
    m_resetPage    = new ResetPasswordPage(this);

    // Make the stack transparent so the painted gradient shows through
    m_stack->setAutoFillBackground(false);

    m_stack->addWidget(m_loginPage);    // 0
    m_stack->addWidget(m_registerPage); // 1
    m_stack->addWidget(m_resetPage);    // 2

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stack);

    // ── Navigation ───────────────────────────────────────────────────────
    connect(m_loginPage,    &LoginPage::navigateToRegister,   this, &MainWindow::showRegister);
    connect(m_loginPage,    &LoginPage::navigateToReset,      this, &MainWindow::showReset);
    connect(m_loginPage,    &LoginPage::loginSuccess,         this, &MainWindow::onLoginSuccess);

    connect(m_registerPage, &RegisterPage::navigateToLogin,   this, &MainWindow::showLogin);
    connect(m_registerPage, &RegisterPage::registerSuccess,   this, &MainWindow::onRegisterSuccess);

    connect(m_resetPage,    &ResetPasswordPage::navigateToLogin, this, &MainWindow::showLogin);
}

void MainWindow::applyAppStyle()
{
    setStyleSheet(loadAppStyle());
}

// ---------------------------------------------------------------------------
// Light green gradient with a soft white radial highlight.
// ---------------------------------------------------------------------------
void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Base: light green diagonal gradient
    QLinearGradient base(0, 0, width(), height());
    base.setColorAt(0.0,  QColor("#C8E6C9"));
    base.setColorAt(0.45, QColor("#E8F5E9"));
    base.setColorAt(1.0,  QColor("#DCEDC8"));
    p.fillRect(rect(), base);

    // Soft white radial highlight — upper portion
    QRadialGradient hi(width() * 0.38f, height() * 0.10f, width() * 0.72f);
    hi.setColorAt(0.0, QColor(255, 255, 255, 95));
    hi.setColorAt(1.0, QColor(255, 255, 255, 0));
    p.fillRect(rect(), hi);
}

// ---------------------------------------------------------------------------
void MainWindow::showLogin()    { m_stack->setCurrentIndex(0); }
void MainWindow::showRegister() { m_stack->setCurrentIndex(1); }
void MainWindow::showReset()    { m_stack->setCurrentIndex(2); }

void MainWindow::onLoginSuccess(const QString &username)
{
    QMessageBox box(this);
    box.setWindowTitle("Welcome back!");
    box.setIcon(QMessageBox::NoIcon);
    box.setText(
        QString("<b style='font-size:16px'>Welcome, %1!</b><br><br>"
                "You're signed in to <b>Book Club</b>.<br>"
                "This is where your main app would launch.")
            .arg(username.toHtmlEscaped())
    );
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

void MainWindow::onRegisterSuccess(const QString &username)
{
    m_stack->setCurrentIndex(0);
    QMessageBox box(this);
    box.setWindowTitle("Account created");
    box.setIcon(QMessageBox::NoIcon);
    box.setText(
        QString("<b style='font-size:15px'>You're in!</b><br><br>"
                "Welcome to Book Club, <b>%1</b>.<br>"
                "Sign in with your new credentials.")
            .arg(username.toHtmlEscaped())
    );
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}
