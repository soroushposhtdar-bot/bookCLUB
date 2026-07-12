// =============================================================================
//  tests/server/test_auth.cpp
// =============================================================================
//  Unit tests for the auth subsystem. Uses Qt Test framework.
//
//  Covers:
//    • PasswordHasher — hash format, verify round-trip, wrong password rejection
//    • AuthService    — login success/failure, blocked users, registration,
//                       username availability, security-question verification,
//                       reset-token issuance & password reset, genre selection
//
//  Build (standalone):
//    cd build && cmake --build . --target test_auth
//  Run:
//    ./test_auth              # Qt Test auto-runs all slots
//
//  Or via CTest (if the project wires tests up via add_test()):
//    ctest -R test_auth --output-on-failure
// =============================================================================
#include <QtTest/QtTest>

#include "common/Utils/PasswordHasher.h"
#include "services/AuthService.h"

#include <memory>

using bookclub::common::PasswordHasher;
using bookclub::client::AuthService;

// =============================================================================
//  PasswordHasher test fixture
// =============================================================================
class TestPasswordHasher : public QObject {
    Q_OBJECT
private slots:
    void hashProducesSaltDollarHashFormat();
    void verifyAcceptsCorrectPassword();
    void verifyRejectsWrongPassword();
    void hashIsSaltedAndUnique();
    void verifyRejectsMalformedStoredHash();
};

void TestPasswordHasher::hashProducesSaltDollarHashFormat() {
    const QString h = PasswordHasher::hash("password123");
    QVERIFY(h.contains('$'));
    const auto parts = h.split('$');
    QCOMPARE(parts.size(), 2);
    QVERIFY(!parts[0].isEmpty());   // salt
    QVERIFY(!parts[1].isEmpty());   // sha256 hex digest
}

void TestPasswordHasher::verifyAcceptsCorrectPassword() {
    const QString stored = PasswordHasher::hash("s3cret-Pa55");
    QVERIFY(PasswordHasher::verify("s3cret-Pa55", stored));
}

void TestPasswordHasher::verifyRejectsWrongPassword() {
    const QString stored = PasswordHasher::hash("correct-password");
    QVERIFY(!PasswordHasher::verify("wrong-password", stored));
    QVERIFY(!PasswordHasher::verify("", stored));
    QVERIFY(!PasswordHasher::verify("correct-password ", stored));   // trailing space
}

void TestPasswordHasher::hashIsSaltedAndUnique() {
    // Same password hashed twice should produce different stored values
    // (because the salt is random) but both should verify against the
    // original password.
    const QString a = PasswordHasher::hash("dup-password");
    const QString b = PasswordHasher::hash("dup-password");
    QVERIFY(a != b);
    QVERIFY(PasswordHasher::verify("dup-password", a));
    QVERIFY(PasswordHasher::verify("dup-password", b));
}

void TestPasswordHasher::verifyRejectsMalformedStoredHash() {
    QVERIFY(!PasswordHasher::verify("anything", ""));             // empty
    QVERIFY(!PasswordHasher::verify("anything", "noseparator"));  // no $
    QVERIFY(!PasswordHasher::verify("anything", "$"));            // empty salt+hash
}

// =============================================================================
//  AuthService test fixture
// =============================================================================
class TestAuthService : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void loginSucceedsForSeededUser();
    void loginFailsWithWrongPassword();
    void loginFailsForUnknownUser();
    void loginFailsForBlockedUser();
    void logoutClearsCurrentUser();

    void registerUserSucceedsForAvailableUsername();
    void registerUserFailsForTakenUsername();
    void isUsernameAvailableReflectsRegistration();
    void requiresGenreSetupIsTrueForNewlyRegistered();

    void securityQuestionForReturnsStoredQuestion();
    void verifySecurityAnswerIsCaseInsensitive();
    void verifySecurityAnswerRejectsWrongAnswer();

    void issueResetTokenReturnsNonEmptyTokenForKnownUser();
    void issueResetTokenReturnsEmptyForUnknownUser();
    void resetPasswordSucceedsWithValidToken();
    void resetPasswordFailsWithInvalidToken();
    void resetPasswordClearsTokenAfterUse();

    void saveGenreSelectionPersistsGenres();
    void saveGenreSelectionClearsRequiresFlag();

private:
    std::unique_ptr<AuthService> _svc;
    QString _capturedError;
};

void TestAuthService::initTestCase() {
    _svc = std::make_unique<AuthService>();
    QVERIFY(_svc != nullptr);
    // Pre-seeded demo accounts come from AuthService's constructor.
    QVERIFY(_svc->userExists("alice"));
    QVERIFY(_svc->userExists("bob"));
    QVERIFY(_svc->userExists("admin"));
    QVERIFY(_svc->userExists("publisher"));
    QVERIFY(_svc->userExists("server"));
    QVERIFY(_svc->userExists("blocked"));
}

void TestAuthService::cleanupTestCase() {
    _svc.reset();
}

// ---- Login ----

void TestAuthService::loginSucceedsForSeededUser() {
    QString err;
    QVERIFY(_svc->login("alice", "password123", err));
    QVERIFY(err.isEmpty());
    QCOMPARE(_svc->currentUsername(), QString("alice"));
    QCOMPARE(_svc->currentRole(), QString("user"));
    QVERIFY(_svc->isLoggedIn());
    _svc->logout();
}

void TestAuthService::loginFailsWithWrongPassword() {
    QString err;
    QVERIFY(!_svc->login("alice", "wrong-password", err));
    QVERIFY(!err.isEmpty());
    QVERIFY(!_svc->isLoggedIn());
}

void TestAuthService::loginFailsForUnknownUser() {
    QString err;
    QVERIFY(!_svc->login("nonexistent", "anything", err));
    QVERIFY(err.contains("No account", Qt::CaseInsensitive));
    QVERIFY(!_svc->isLoggedIn());
}

void TestAuthService::loginFailsForBlockedUser() {
    QString err;
    QVERIFY(!_svc->login("blocked", "password123", err));
    QVERIFY(err.contains("blocked", Qt::CaseInsensitive));
    QVERIFY(!_svc->isLoggedIn());
}

void TestAuthService::logoutClearsCurrentUser() {
    QString err;
    QVERIFY(_svc->login("bob", "password123", err));
    QVERIFY(_svc->isLoggedIn());
    _svc->logout();
    QVERIFY(!_svc->isLoggedIn());
    QVERIFY(_svc->currentUsername().isEmpty());
    QVERIFY(_svc->currentRole().isEmpty());
}

// ---- Registration ----

void TestAuthService::registerUserSucceedsForAvailableUsername() {
    QString err;
    const bool ok = _svc->registerUser(
        "newuser1", "New User", "newpass123",
        "What was the name of your first pet?", "rex", err);
    QVERIFY(ok);
    QVERIFY(err.isEmpty());
    QVERIFY(_svc->userExists("newuser1"));
    QVERIFY(!_svc->isUsernameAvailable("newuser1"));
}

void TestAuthService::registerUserFailsForTakenUsername() {
    QString err;
    const bool ok = _svc->registerUser(
        "alice", "Alice Imposter", "whatever",
        "What was the name of your first pet?", "x", err);
    QVERIFY(!ok);
    QVERIFY(err.contains("already taken", Qt::CaseInsensitive));
}

void TestAuthService::isUsernameAvailableReflectsRegistration() {
    QVERIFY(_svc->isUsernameAvailable("brand-new-user"));
    QVERIFY(!_svc->isUsernameAvailable("alice"));
    QVERIFY(!_svc->isUsernameAvailable("ALICE"));   // case-insensitive
    QVERIFY(!_svc->isUsernameAvailable("  alice  "));  // trimmed
    QVERIFY(!_svc->isUsernameAvailable(""));        // empty → not available
}

void TestAuthService::requiresGenreSetupIsTrueForNewlyRegistered() {
    QString err;
    QVERIFY(_svc->registerUser("genreuser", "Genre User", "pass1234",
                                "In what city were you born?", "tehran", err));
    QVERIFY(_svc->requiresGenreSetup("genreuser"));
    // Seeded demo users have requiresGenreSetup=false (they were pre-onboarded).
    QVERIFY(!_svc->requiresGenreSetup("alice"));
}

// ---- Security question / answer ----

void TestAuthService::securityQuestionForReturnsStoredQuestion() {
    const QString q = _svc->securityQuestionFor("alice");
    QCOMPARE(q, QString("What was the name of your first pet?"));
    QVERIFY(_svc->securityQuestionFor("nonexistent").isEmpty());
}

void TestAuthService::verifySecurityAnswerIsCaseInsensitive() {
    // alice's answer is "whiskers" — should accept any case
    QVERIFY(_svc->verifySecurityAnswer("alice", "whiskers"));
    QVERIFY(_svc->verifySecurityAnswer("alice", "WHISKERS"));
    QVERIFY(_svc->verifySecurityAnswer("alice", "WhIsKeRs"));
    // With surrounding whitespace
    QVERIFY(_svc->verifySecurityAnswer("alice", "  whiskers  "));
}

void TestAuthService::verifySecurityAnswerRejectsWrongAnswer() {
    QVERIFY(!_svc->verifySecurityAnswer("alice", "rex"));
    QVERIFY(!_svc->verifySecurityAnswer("alice", ""));
    QVERIFY(!_svc->verifySecurityAnswer("nonexistent", "anything"));
}

// ---- Reset token / password reset ----

void TestAuthService::issueResetTokenReturnsNonEmptyTokenForKnownUser() {
    const QString token = _svc->issueResetToken("alice");
    QVERIFY(!token.isEmpty());
    QCOMPARE(token.length(), 64);   // 32 bytes hex = 64 chars
}

void TestAuthService::issueResetTokenReturnsEmptyForUnknownUser() {
    const QString token = _svc->issueResetToken("does-not-exist");
    QVERIFY(token.isEmpty());
}

void TestAuthService::resetPasswordSucceedsWithValidToken() {
    // Arrange
    const QString token = _svc->issueResetToken("bob");
    QVERIFY(!token.isEmpty());

    // Act
    QString err;
    const bool ok = _svc->resetPassword("bob", token, "newPass-456", err);

    // Assert
    QVERIFY(ok);
    QVERIFY(err.isEmpty());

    // The new password should now log in
    QString loginErr;
    QVERIFY(_svc->login("bob", "newPass-456", loginErr));
    _svc->logout();
}

void TestAuthService::resetPasswordFailsWithInvalidToken() {
    QString err;
    const bool ok = _svc->resetPassword("alice", "bogus-token", "newPass-789", err);
    QVERIFY(!ok);
    QVERIFY(err.contains("token", Qt::CaseInsensitive));

    // The old password should still work (reset failed)
    QString loginErr;
    QVERIFY(_svc->login("alice", "password123", loginErr));
    _svc->logout();
}

void TestAuthService::resetPasswordClearsTokenAfterUse() {
    // Issue a token, use it successfully, then try to reuse it — should fail.
    const QString token = _svc->issueResetToken("admin");
    QString err1;
    QVERIFY(_svc->resetPassword("admin", token, "firstNewPass-1", err1));

    QString err2;
    QVERIFY(!_svc->resetPassword("admin", token, "secondNewPass-2", err2));
    QVERIFY(err2.contains("token", Qt::CaseInsensitive));
}

// ---- Genre selection ----

void TestAuthService::saveGenreSelectionPersistsGenres() {
    QString err;
    QVERIFY(_svc->registerUser("genreuser2", "Genre User 2", "pass1234",
                                "What was the name of your first pet?", "rex", err));

    const QStringList picked = { "Fiction", "Mystery", "Fantasy" };
    QVERIFY(_svc->saveGenreSelection("genreuser2", picked));
    QVERIFY(!_svc->requiresGenreSetup("genreuser2"));
}

void TestAuthService::saveGenreSelectionClearsRequiresFlag() {
    QString err;
    QVERIFY(_svc->registerUser("genreuser3", "Genre User 3", "pass1234",
                                "What was the name of your first pet?", "rex", err));
    QVERIFY(_svc->requiresGenreSetup("genreuser3"));

    QVERIFY(_svc->saveGenreSelection("genreuser3", { "Poetry" }));
    QVERIFY(!_svc->requiresGenreSetup("genreuser3"));
}

// =============================================================================
//  Test main — runs all test fixtures via Qt Test
// =============================================================================
//  We need to run BOTH TestPasswordHasher and TestAuthService. QTEST_MAIN
//  only generates a main for one class, so we use QTEST_APPLESS_MAIN (no
//  QCoreApplication needed for these pure-logic tests) and manually
//  instantiate the other fixture inside main.
// =============================================================================
QT_BEGIN_NAMESPACE
// Helper: run a QObject test fixture, return number of failures.
static int _runTest(int argc, char* argv[], QObject* fixture) {
    return QTest::qExec(fixture, argc, argv);
}
QT_END_NAMESPACE

int main(int argc, char* argv[]) {
    int failures = 0;

    TestPasswordHasher passwordHasherTests;
    failures += _runTest(argc, argv, &passwordHasherTests);

    TestAuthService authServiceTests;
    failures += _runTest(argc, argv, &authServiceTests);

    return failures;
}

#include "test_auth.moc"
