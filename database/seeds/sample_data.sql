-- ============================================================
-- داده‌های نمونه برای تست bookCLUB
--
-- همه‌ی passwordHash و securityAnswerHash با همان الگوریتمی که
-- PasswordHasher::hash در C++ استفاده می‌کند تولید شده‌اند:
--   salt = 16 کاراکتر تصادفی الفبایی-عددی
--   hash = sha256(salt + plain).hex()
--   رشته‌ی نهایی: "salt$hash"
--
-- برای ساخت هش تازه می‌توانید از scripts/gen_hash.py استفاده کنید:
--   python3 scripts/gen_hash.py mypassword
-- ============================================================

-- ---- ۱. کاربران نمونه ----
-- username / password / securityAnswer (به حروف کوچک)
--   admin       / admin        / "shahnameh"
--   publisher1  / publisher1   / "saadi"
--   amir        / amir1234     / "mollanasreddin"
INSERT INTO Users (id, username, passwordHash, displayName, email, phone, securityQuestion, securityAnswerHash, status, role, createdAt, updatedAt)
VALUES
    ('user-001', 'admin',      'bcz2gqhFWDzbYm1k$bd7c8cb3a49f02e3f2d9c6e916ce01e23d2ef9c1eb1cedc69ba53fbf09cedd43', 'مدیر سیستم',   'admin@bookclub.com',    '09121111111', 'نام کتاب مورد علاقه؟',    'bcz2gqhFWDzbYm1k$bd7c8cb3a49f02e3f2d9c6e916ce01e23d2ef9c1eb1cedc69ba53fbf09cedd43', 1, 2, datetime('now'), datetime('now')),
    ('user-002', 'publisher1', 'T5M6J8xFfyZ6151E$8c8964ba8a2e4a85e32742a7da04212c42141682ca3a6c21a60357b9e1cecd05', 'ناشر اول',     'pub1@bookclub.com',     '09122222222', 'نام نویسنده مورد علاقه؟', 'T5M6J8xFfyZ6151E$8c8964ba8a2e4a85e32742a7da04212c42141682ca3a6c21a60357b9e1cecd05', 1, 1, datetime('now'), datetime('now')),
    ('user-003', 'amir',       'aUgjpCxaPMfrAmEW$cd172e7a065e17c7c1747c19b86bc3a362bd957aaf3efc51b05248d20997251c', 'امیر حسینی',   'amir@bookclub.com',     '09123333333', 'کتاب مورد علاقه؟',        'aUgjpCxaPMfrAmEW$cd172e7a065e17c7c1747c19b86bc3a362bd957aaf3efc51b05248d20997251c', 1, 0, datetime('now'), datetime('now'));

-- ---- ۲. ژانرهای نمونه ----
INSERT INTO Genres (id, name, aliases) VALUES
    ('genre-001', 'برنامه‌نویسی', '[]'),
    ('genre-002', 'رمان',         '[]'),
    ('genre-003', 'تاریخی',       '[]')
ON CONFLICT(name) DO NOTHING;

-- ---- ۳. کتاب‌های نمونه ----
INSERT INTO Books (id, title, authorName, publisherId, genreIds, description, coverImagePath, pdfFilePath, basePrice, discountValue, averageRating, ratingCount, totalSales, stockCount, isActive, visibility, availability, createdAt, updatedAt)
VALUES
    ('book-001', 'برنامه‌نویسی با Qt',    'سعید محمدی', 'user-002', '["genre-001"]', 'کتاب کامل آموزش Qt',       '/images/qt_book.jpg',  '/pdfs/qt_book.pdf',  35.0, 5.0, 4.5, 10, 20, 50, 1, 0, 0, datetime('now'), datetime('now')),
    ('book-002', 'رازهای سی‌پلاس‌پلاس',   'مریم احمدی', 'user-002', '["genre-001"]', 'مفاهیم پیشرفته C++',       '/images/cpp_book.jpg', '/pdfs/cpp_book.pdf', 45.0, 0.0, 4.8,  8, 15, 30, 1, 0, 0, datetime('now'), datetime('now')),
    ('book-003', 'شاهنامه برای همه',      'ابوالقاسم فردوسی', 'user-002', '["genre-003"]', 'گزارشی ساده از داستان‌های شاهنامه', '/images/shahnameh.jpg', '/pdfs/shahnameh.pdf', 60.0, 0.0, 4.9, 25, 100, 200, 1, 0, 0, datetime('now'), datetime('now'));

-- ---- ۴. نظرات نمونه ----
INSERT INTO Reviews (id, bookId, userId, userDisplayName, text, stars, createdAt, updatedAt, isEdited)
VALUES
    ('rev-001', 'book-001', 'user-003', 'امیر حسینی', 'کتاب فوق‌العاده‌ای بود!', 5, datetime('now'), datetime('now'), 0),
    ('rev-002', 'book-002', 'user-003', 'امیر حسینی', 'بسیار مفید و روان',     4, datetime('now'), datetime('now'), 0);

-- ---- ۵. قفسه‌های نمونه ----
INSERT INTO Shelves (id, userId, name, description, bookIds, isSystemShelf)
VALUES
    ('shelf-001', 'user-003', 'کتاب‌های خوانده‌شده', 'کتاب‌هایی که کامل خوانده‌ام', '["book-001"]', 0),
    ('shelf-002', 'user-003', 'علاقه‌مندی‌ها',     'کتاب‌های مورد علاقه',        '["book-002"]', 0);

-- ---- ۶. اعلان‌های نمونه ----
INSERT INTO Notifications (id, receiverUserId, title, message, relatedEntityId, type, state, createdAt, readAt)
VALUES
    ('notif-001', 'user-003', 'کتاب جدید', 'کتاب برنامه‌نویسی با Qt منتشر شد!', 'book-001', 1, 0, datetime('now'), NULL);
