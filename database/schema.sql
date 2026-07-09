-- ============================================================
-- کتابخانه الکترونیکی bookCLUB - اسکریپت دیتابیس SQLite
-- ============================================================

-- فعال‌سازی کلیدهای خارجی (Foreign Keys)
PRAGMA foreign_keys = ON;

-- ============================================================
-- ۱. جدول کاربران (Users)
-- ============================================================
CREATE TABLE IF NOT EXISTS Users (
    id                  TEXT PRIMARY KEY,          -- UUID
    username            TEXT UNIQUE NOT NULL,      -- نام کاربری یکتا
    passwordHash        TEXT NOT NULL,             -- هش رمز عبور (با salt)
    displayName         TEXT NOT NULL,             -- نام نمایشی
    email               TEXT NOT NULL,             -- ایمیل
    phone               TEXT,                      -- شماره تلفن
    securityQuestion    TEXT NOT NULL,             -- سوال امنیتی
    securityAnswerHash  TEXT NOT NULL,             -- هش پاسخ سوال امنیتی
    status              INTEGER NOT NULL DEFAULT 0,-- 0:Pending, 1:Active, 2:Blocked, 3:Disabled, 4:Deleted
    role                INTEGER NOT NULL DEFAULT 0,-- 0:User, 1:Publisher, 2:Admin
    createdAt           DATETIME NOT NULL,         -- زمان ثبت‌نام
    updatedAt           DATETIME NOT NULL          -- آخرین بروزرسانی
);

-- ============================================================
-- ۲. جدول کتاب‌ها (Books)
-- ============================================================
CREATE TABLE IF NOT EXISTS Books (
    id                  TEXT PRIMARY KEY,          -- UUID
    title               TEXT NOT NULL,             -- عنوان کتاب
    authorName          TEXT NOT NULL,             -- نام نویسنده
    publisherId         TEXT NOT NULL,             -- شناسه‌ی ناشر (ارجاع به Users)
    genreIds            TEXT,                      -- آرایه‌ی JSON از شناسه‌های ژانر
    description         TEXT,                      -- توضیحات
    coverImagePath      TEXT,                      -- مسیر عکس جلد
    pdfFilePath         TEXT NOT NULL,             -- مسیر فایل PDF
    basePrice           REAL NOT NULL DEFAULT 0.0, -- قیمت پایه
    discountValue       REAL NOT NULL DEFAULT 0.0, -- مقدار تخفیف
    averageRating       REAL NOT NULL DEFAULT 0.0, -- میانگین امتیاز
    ratingCount         INTEGER NOT NULL DEFAULT 0,-- تعداد امتیازها
    totalSales          INTEGER NOT NULL DEFAULT 0,-- تعداد فروش
    stockCount          INTEGER NOT NULL DEFAULT 0,-- موجودی
    isActive            INTEGER NOT NULL DEFAULT 1,-- 1:فعال, 0:غیرفعال
    visibility          INTEGER NOT NULL DEFAULT 0,-- 0:Visible, 1:Hidden, 2:Archived
    availability        INTEGER NOT NULL DEFAULT 0,-- 0:Available, 1:Unavailable, 2:Removed
    createdAt           DATETIME NOT NULL,         -- زمان انتشار
    updatedAt           DATETIME NOT NULL,         -- آخرین بروزرسانی
    FOREIGN KEY (publisherId) REFERENCES Users(id) ON DELETE CASCADE
);

-- ============================================================
-- ۳. جدول سفارشات (Orders)
-- ============================================================
CREATE TABLE IF NOT EXISTS Orders (
    id                  TEXT PRIMARY KEY,          -- UUID
    userId              TEXT NOT NULL,             -- شناسه‌ی کاربر خریدار
    items               TEXT NOT NULL,             -- آرایه‌ی JSON از آیتم‌ها
    subtotal            REAL NOT NULL DEFAULT 0.0, -- جمع قیمت قبل از تخفیف
    discountTotal       REAL NOT NULL DEFAULT 0.0, -- جمع تخفیف
    finalTotal          REAL NOT NULL DEFAULT 0.0, -- مبلغ نهایی پرداختی
    paid                INTEGER NOT NULL DEFAULT 0,-- 0:پرداخت نشده, 1:پرداخت شده
    completed           INTEGER NOT NULL DEFAULT 0,-- 0:ناقص, 1:تکمیل شده
    createdAt           DATETIME NOT NULL,         -- زمان ثبت سفارش
    FOREIGN KEY (userId) REFERENCES Users(id) ON DELETE CASCADE
);

-- ============================================================
-- ۴. جدول نظرات (Reviews)
-- ============================================================
CREATE TABLE IF NOT EXISTS Reviews (
    id                  TEXT PRIMARY KEY,          -- UUID
    bookId              TEXT NOT NULL,             -- شناسه‌ی کتاب
    userId              TEXT NOT NULL,             -- شناسه‌ی کاربر
    userDisplayName     TEXT NOT NULL,             -- نام نمایشی کاربر (برای سرعت)
    text                TEXT NOT NULL,             -- متن نظر
    stars               INTEGER NOT NULL DEFAULT 5,-- امتیاز از ۱ تا ۵
    createdAt           DATETIME NOT NULL,         -- زمان ثبت
    updatedAt           DATETIME NOT NULL,         -- آخرین ویرایش
    isEdited            INTEGER NOT NULL DEFAULT 0,-- 0:ویرایش نشده, 1:ویرایش شده
    FOREIGN KEY (bookId) REFERENCES Books(id) ON DELETE CASCADE,
    FOREIGN KEY (userId) REFERENCES Users(id) ON DELETE CASCADE
);

-- ============================================================
-- ۵. جدول اعلان‌ها (Notifications)
-- ============================================================
CREATE TABLE IF NOT EXISTS Notifications (
    id                  TEXT PRIMARY KEY,          -- UUID
    receiverUserId      TEXT NOT NULL,             -- گیرنده‌ی اعلان
    title               TEXT NOT NULL,             -- عنوان
    message             TEXT NOT NULL,             -- متن
    relatedEntityId     TEXT,                      -- شناسه‌ی موجودیت مرتبط (مثلاً bookId)
    type                INTEGER NOT NULL DEFAULT 0,-- 0:Generic, 1:NewBook, 2:Discount, 3:Sale, 4:Review, ...
    state               INTEGER NOT NULL DEFAULT 0,-- 0:Unread, 1:Read, 2:Archived
    createdAt           DATETIME NOT NULL,         -- زمان ایجاد
    readAt              DATETIME,                  -- زمان خوانده‌شدن
    FOREIGN KEY (receiverUserId) REFERENCES Users(id) ON DELETE CASCADE
);

-- ============================================================
-- ۶. جدول قفسه‌های کتابخانه (Shelves)
-- ============================================================
CREATE TABLE IF NOT EXISTS Shelves (
    id                  TEXT PRIMARY KEY,          -- UUID
    userId              TEXT NOT NULL,             -- مالک قفسه
    name                TEXT NOT NULL,             -- نام قفسه (مثلاً "رمان‌های عاشقانه")
    description         TEXT,                      -- توضیحات
    bookIds             TEXT,                      -- آرایه‌ی JSON از شناسه‌های کتاب
    isSystemShelf       INTEGER NOT NULL DEFAULT 0,-- 0:دست‌ساز, 1:سیستمی (مثل "همه کتاب‌ها")
    FOREIGN KEY (userId) REFERENCES Users(id) ON DELETE CASCADE
);

-- ============================================================
-- ۷. جدول تخفیف‌های زمان‌دار (Discounts)
-- ============================================================
CREATE TABLE IF NOT EXISTS Discounts (
    id                  TEXT PRIMARY KEY,          -- UUID
    bookId              TEXT NOT NULL,             -- کتاب مورد نظر
    type                INTEGER NOT NULL DEFAULT 0,-- 0:درصدی, 1:مبلغ ثابت
    value               REAL NOT NULL DEFAULT 0.0, -- مقدار تخفیف
    startsAt            DATETIME NOT NULL,         -- زمان شروع
    endsAt              DATETIME NOT NULL,         -- زمان پایان
    isActive            INTEGER NOT NULL DEFAULT 1,-- 1:فعال, 0:غیرفعال
    FOREIGN KEY (bookId) REFERENCES Books(id) ON DELETE CASCADE
);

-- ============================================================
-- ۸. جدول جلسات مطالعه‌ی گروهی (StudySessions)
-- ============================================================
CREATE TABLE IF NOT EXISTS StudySessions (
    id                  TEXT PRIMARY KEY,          -- UUID
    bookId              TEXT NOT NULL,             -- کتاب مورد مطالعه
    hostUserId          TEXT NOT NULL,             -- میزبان جلسه
    participantUserIds  TEXT,                      -- آرایه‌ی JSON از شناسه‌های شرکت‌کنندگان
    state               INTEGER NOT NULL DEFAULT 0,-- 0:Created, 1:Active, 2:Paused, 3:Closed
    currentPage         INTEGER NOT NULL DEFAULT 0,-- صفحه‌ی فعلی
    zoomLevel           REAL NOT NULL DEFAULT 1.0,-- سطح بزرگ‌نمایی
    synced              INTEGER NOT NULL DEFAULT 0,-- 0:همگام نشده, 1:همگام شده
    createdAt           DATETIME NOT NULL,         -- زمان ایجاد
    FOREIGN KEY (bookId) REFERENCES Books(id) ON DELETE CASCADE,
    FOREIGN KEY (hostUserId) REFERENCES Users(id) ON DELETE CASCADE
);

-- ============================================================
-- ۹. جدول ژانرها (Genres) - برای مدیریت ژانرها
-- ============================================================
CREATE TABLE IF NOT EXISTS Genres (
    id                  TEXT PRIMARY KEY,          -- UUID
    name                TEXT UNIQUE NOT NULL,      -- نام ژانر (مثلاً "رمان")
    aliases             TEXT                       -- نام‌های مستعار (JSON array)
);

-- ============================================================
-- ۱۰. جدول نشست‌های کاربر (Sessions) - برای مدیریت توکن
-- ============================================================
CREATE TABLE IF NOT EXISTS Sessions (
    id                  TEXT PRIMARY KEY,          -- UUID
    userId              TEXT NOT NULL,             -- شناسه‌ی کاربر
    token               TEXT NOT NULL,             -- توکن نشست
    expiresAt           DATETIME NOT NULL,         -- زمان انقضا
    createdAt           DATETIME NOT NULL,         -- زمان ایجاد
    FOREIGN KEY (userId) REFERENCES Users(id) ON DELETE CASCADE
);

-- ============================================================
-- ایجاد ایندکس‌ها برای بهبود سرعت
-- ============================================================
CREATE INDEX idx_users_username ON Users(username);
CREATE INDEX idx_books_publisherId ON Books(publisherId);
CREATE INDEX idx_books_title ON Books(title);
CREATE INDEX idx_books_authorName ON Books(authorName);
CREATE INDEX idx_orders_userId ON Orders(userId);
CREATE INDEX idx_reviews_bookId ON Reviews(bookId);
CREATE INDEX idx_reviews_userId ON Reviews(userId);
CREATE INDEX idx_notifications_receiverUserId ON Notifications(receiverUserId);
CREATE INDEX idx_shelves_userId ON Shelves(userId);
CREATE INDEX idx_discounts_bookId ON Discounts(bookId);
CREATE INDEX idx_studysessions_bookId ON StudySessions(bookId);
