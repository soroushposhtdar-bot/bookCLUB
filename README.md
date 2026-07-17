# BookCLUB Backend — V2 (Fixed)

این نسخه، نسخه‌ی اصلاح‌شده‌ی `BookCLUB V1` است. تمام باگ‌های مسدودکننده‌ی
 کامپایل و اتصال برطرف شده‌اند و مجموعه تست‌های end-to-end روی بک‌اند
 بدون نیاز به UI با موفقیت اجرا می‌شود. این نسخه آماده‌ی اتصال به لایه‌ی
 UI (Qt Widgets/QML) است.

## نحوه‌ی build و run

### پیش‌نیازها
- C++17 compiler (g++ ≥ 9 یا Clang ≥ 10)
- CMake ≥ 3.16
- Qt 5.15 با این ماژول‌ها: Core، Network، Sql
- پلاگین `libqsqlite.so` (بسته‌ی `libqt5sql5-sqlite` در دبیان/اوبونتو)

### Build
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```
باینری‌ها در `build/bin/BookClubServer` و `build/bin/BookClubClient` ساخته می‌شوند.

### Run
```bash
# سرور (از ریشه‌ی پروژه اجرا شود تا database/schema.sql پیدا شود)
cd BookCLUB\ V2
./build/bin/BookClubServer --database bookclub.db --port 8080

# کلاینت (در ترمینال دیگر)
./build/bin/BookClubClient --user amir --pass amir1234
```

### کاربران نمونه (در `database/seeds/sample_data.sql`)
| username    | password    | role     |
|-------------|-------------|----------|
| admin       | admin       | Admin    |
| publisher1  | publisher1  | Publisher|
| amir        | amir1234    | User     |

برای ساختن هش رمزعبور جدید:
```bash
python3 scripts/gen_hash.py mypassword
# خروجی: salt$hash
```

## تست خودکار
```bash
# سرور را اجرا کنید، سپس:
python3 scripts/e2e_test.py
```
این اسکریپت ۱۴ سناریو را اجرا می‌کند: login برای سه نقش، GetHomeSections،
SearchBooks، GetBookDetails، AddToCart، GetCart، Checkout، GetLibrary،
CreateShelf، AddBookToShelf، GetNotifications، GetPublisherAnalytics،
GetUsersList.

## خلاصه‌ی باگ‌های اصلاح‌شده

### ۱) باگ‌های کامپایل
- **حساسیت به حروف بزرگ/کوچک در include** — تمام فایل‌های
  `common/Interfaces/*.cpp` از مسیرهای lowercase مثل
  `common/interfaces/...`، `common/models/...`، `common/utils/...` استفاده
  می‌کردند در حالی که پوشه‌ها Capital هستند. روی لینوکس (که file system
  case-sensitive است) کامپایل شکست می‌خورد.
- **`Book::setActive(bool)` وجود نداشت** — `IBookRepository.cpp` از این
  متد استفاده می‌کرد در حالی که کلاس `Book` فقط `activate()` و
  `deactivate()` داشت. به جای آن از این دو متد استفاده می‌کنیم.
- **`PasswordHasher::generateSalt` و `hashPassword` وجود نداشتند** —
  API واقعی `hash(password)` و `verify(password, storedHash)` است.
  `IAuthService.cpp` به API قدیمی گره خورده بود و ضمناً verification
  کاملاً شکسته بود (هش salt-دار با هش salt-less مقایسه می‌شد که هیچ‌وقت
  برابر نمی‌شد).
- **`const QSqlQuery&` با `next()` غیر-const** — چندین helper در
  repository‌ها `const QSqlQuery&` می‌گرفتند و سپس `query.next()` را
  صدا می‌زدند. به‌خاطر const-correctness کامپایل نمی‌شد.
- **`QJsonObject` و `QVariantList` ناقص** — `IOrderRepository.cpp` و
  `INotificationRepository.cpp` هدرهای لازم را include نکرده بودند.
- **اعلان factory functions وجود نداشت** — توابع `createAuthService()`،
  `createUserRepository()` و غیره در `.cpp`‌ها تعریف شده بودند اما در
  هدرها اعلان نشده بودند، پس `ServerCore.cpp` آن‌ها را پیدا نمی‌کرد.

### ۲) باگ‌های runtime
- **`QSqlDatabase: QSQLITE driver not loaded`** — پلاگین sqlite روی
  مسیر پیش‌فرض نبود. اصلاح: `QT_PLUGIN_PATH` باید به مسیر plugins
  qt5 تنظیم شود.
- **`schema.sql` پیدا نمی‌شد** — `DatabaseManager::runSchemaScript` از
  مسیر hardcode‌شده‌ی `../database/schema.sql` استفاده می‌کرد. اصلاح:
  تابع `locateDatabaseFile` چندین مسیر کاندید را امتحان می‌کند.
- **اجرای چند-statement SQL** — `DatabaseManager` قبلی `schema.sql` را
  با یک `QSqlQuery::exec()` اجرا می‌کرد که فقط یک statement اجرا می‌کند.
  اصلاح: `executeSqlScript` statements را با `;` split کرده و
  comment‌های `--` را هم حذف می‌کند.
- **اجرای دوبل schema در `ServerCore::setupDatabase`** — این تابع کل
  `schema.sql` را به‌عنوان یک exec می‌زد که سایلنت fail می‌شد. حذف شد؛
  `DatabaseManager` به‌تنهایی مسئول schema و seed است.
- **`QSocketNotifier: Invalid socket`** — `ConnectionManager` descriptor
  سوکت را از `nextPendingConnection()` می‌گرفت و یک `QTcpSocket` جدید
  با همان descriptor می‌ساخت. دو آبجکت QTcpSocket روی یک fd → warning و
  نرسیدن داده. اصلاح: `ClientConnection` حالا خود `QTcpSocket*` را
  می‌گیرد و reparent می‌کند.
- **هش‌های seed خیالی** — `'e7c3b8f9...'` با هیچ رمزعبوری match نمی‌شد.
  اصلاح: همه‌ی هش‌ها با `scripts/gen_hash.py` و الگوریتم واقعی
  `PasswordHasher::hash` باز‌تولید شدند.
- **Checkout با 500 fail می‌شد** — دو باگ:
  1. `purchaseCart` در `IBookService` سفارش را save می‌کرد و سپس
     `handleCheckout` دوباره save می‌زد → `UNIQUE constraint failed:
     Orders.id`. اصلاح: `purchaseCart` فقط آبجکت Order می‌سازد و
     `handleCheckout` یک‌بار آن را save می‌کند.
  2. `handleAddToCart` اطلاعات کتاب (title، price) را load نمی‌کرد و
     CartItem با قیمت ۰ ساخته می‌شد → total همیشه ۰. اصلاح: حالا کتاب
     از دیتابیس خوانده می‌شود.
- **کتابخانه همیشه خالی** — `LibraryRequestHandler::getOrCreateLibrary`
  یک `UserLibrary` خالی می‌ساخت و هیچ‌وقت از دیتابیس load نمی‌کرد.
  اصلاح: حالا `purchasedBookIdsFor()` از جدول `Orders.items` استخراج
  می‌شود و `reloadShelves()` قفسه‌ها را از جدول `Shelves` می‌خواند.
  create/delete/add-book نیز به دیتابیس persist می‌شوند.
- **`LibraryShelf` بدون id** — `UserLibrary::createShelf` هیچ‌وقت id
  ست نمی‌کرد → عملیات بعدی روی shelf با `shelfId` همیشه fail می‌شد.
  اصلاح: constructor پیش‌فرض و `createShelf` هر دو یک UUID تولید
  می‌کنند.
- **`m_userToClientMap` هرگز پر نمی‌شد** — `getConnectionByUserId()`
  همیشه nullptr برمی‌گرداند → اعلان‌های real-time به کاربر می‌رسیدند
  اما dispatcher نمی‌توانست socket را پیدا کند. اصلاح: متدهای
  `registerUser`/`unregisterUser` به `ConnectionManager` اضافه شد و
  `ClientConnection::setUserId` آن‌ها را صدا می‌زند.
- **`NotificationDispatcher` هیچ‌وقت instantiate نمی‌شد** — کلاس وجود
  داشت اما `ServerCore` آن را نمی‌ساخت. اصلاح: متصل به `ServerCore`
  و قابل دسترس از طریق `ServerCore::notificationDispatcher()`.
- **use-after-free در `PublisherRequestHandler::handlePublishBook`** —
  `delete book; LOG_INFO(book->title())`. اصلاح: title قبل از delete
  capture می‌شود.
- **سیگنال deprecate‌شده‌ی `QAbstractSocket::error`** — هشدار کامپایل.
  اصلاح: به `&QAbstractSocket::errorOccurred` تغییر یافت.

### ۳) بهبودها
- `database/seeds/sample_data.sql` اکنون شامل ۳ ژانر و ۳ کتاب نمونه
  است (قبلی ۲ کتاب داشت).
- `src/client/main.cpp` اکنون `--host --port --user --pass` را از
  command line می‌پذیرد تا UI به‌سادگی جایگزین کند.
- `scripts/` شامل `gen_hash.py` (تولید هش) و `e2e_test.py` (تست
  end-to-end) است.
- `worklog.md` تاریخچه‌ی کامل تغییرات را نگه می‌دارد.

## معماری

```
┌────────────┐   TCP socket   ┌──────────────┐
│  Client    │ ◄───────────► │   Server     │
│ (Qt App)   │   JSON+length │              │
└─────┬──────┘  prefix        └──────┬───────┘
      │                                │
   Controllers                ConnectionManager
      │                                │
   SessionManager             RequestRouter
      │                                │
   ClientNetworkManager       RequestHandlerBase
                                     │
                              AuthRequestHandler
                              BookRequestHandler
                              CartRequestHandler
                              LibraryRequestHandler
                              PublisherRequestHandler
                              AdminRequestHandler
                              NotificationRequestHandler
                              StudySessionRequestHandler
                                     │
                              IAuthService / IBookService
                              IUserRepository / IBookRepository
                              IOrderRepository / IReviewRepository
                              INotificationRepository
                                     │
                              DatabaseManager (SQLite)
```

## قدم بعدی: اتصال UI

1. لایه‌ی UI (Qt Widgets یا QML) را با `ClientNetworkManager::instance()`
   و کنترلرهای موجود (`AuthController`، `BookCatalogController`،
   `CartController`، `LibraryController`، `PublisherController`،
   `AdminController`، `NotificationController`، `ReaderController`،
   `StudySessionController`) متصل کنید.
2. کنترلرها همگی SIGNAL هایی برای تغییرات state دارند (`loginSucceeded`،
   `featuredBooksChanged`، `cartChanged`، `libraryChanged`، ...) که
   می‌توانید مستقیماً به UI وصل کنید.
3. برای هر request، `ClientNetworkManager::sendRequest(Command, payload)`
   را صدا بزنید و در callback پاسخ را handle کنید.
4. `SessionManager::instance()` اطلاعات نشست جاری (userId، role،
   sessionToken) را نگه می‌دارد.

مثال:
```cpp
// Login flow in UI
AuthController* auth = new AuthController(this);
connect(auth, &AuthController::loginSucceeded, this, &MainWindow::onLoginOk);
connect(auth, &AuthController::loginFailed, this, &MainWindow::onLoginFail);
auth->setLoginCredentials(username, password);
auth->login();
```
