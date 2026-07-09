-- ============================================================
-- داده‌های نمونه برای تست bookCLUB
-- ============================================================

-- ---- ۱. کاربران نمونه ----
INSERT INTO Users (id, username, passwordHash, displayName, email, phone, securityQuestion, securityAnswerHash, status, role, createdAt, updatedAt)
VALUES
    ('user-001', 'admin', 'e7c3b8f9...', 'مدیر سیستم', 'admin@bookclub.com', '09121111111', 'نام کتاب مورد علاقه؟', 'hashed_answer', 1, 2, datetime('now'), datetime('now')),
    ('user-002', 'publisher1', 'e7c3b8f9...', 'ناشر اول', 'pub1@bookclub.com', '09122222222', 'نام نویسنده مورد علاقه؟', 'hashed_answer', 1, 1, datetime('now'), datetime('now')),
    ('user-003', 'amir', 'e7c3b8f9...', 'امیر حسینی', 'amir@bookclub.com', '09123333333', 'کتاب مورد علاقه؟', 'hashed_answer', 1, 0, datetime('now'), datetime('now'));

-- ---- ۲. کتاب‌های نمونه ----
INSERT INTO Books (id, title, authorName, publisherId, genreIds, description, coverImagePath, pdfFilePath, basePrice, discountValue, averageRating, ratingCount, totalSales, stockCount, isActive, visibility, availability, createdAt, updatedAt)
VALUES
    ('book-001', 'برنامه‌نویسی با Qt', 'سعید محمدی', 'user-002', '["genre-001"]', 'کتاب کامل آموزش Qt', '/images/qt_book.jpg', '/pdfs/qt_book.pdf', 35.0, 5.0, 4.5, 10, 20, 50, 1, 0, 0, datetime('now'), datetime('now')),
    ('book-002', 'رازهای سی‌پلاس‌پلاس', 'مریم احمدی', 'user-002', '["genre-002"]', 'مفاهیم پیشرفته C++', '/images/cpp_book.jpg', '/pdfs/cpp_book.pdf', 45.0, 0.0, 4.8, 8, 15, 30, 1, 0, 0, datetime('now'), datetime('now'));

-- ---- ۳. نظرات نمونه ----
INSERT INTO Reviews (id, bookId, userId, userDisplayName, text, stars, createdAt, updatedAt, isEdited)
VALUES
    ('rev-001', 'book-001', 'user-003', 'امیر حسینی', 'کتاب فوق‌العاده‌ای بود!', 5, datetime('now'), datetime('now'), 0),
    ('rev-002', 'book-002', 'user-003', 'امیر حسینی', 'بسیار مفید و روان', 4, datetime('now'), datetime('now'), 0);

-- ---- ۴. قفسه‌های نمونه ----
INSERT INTO Shelves (id, userId, name, description, bookIds, isSystemShelf)
VALUES
    ('shelf-001', 'user-003', 'کتاب‌های خوانده‌شده', 'کتاب‌هایی که کامل خوانده‌ام', '["book-001"]', 0),
    ('shelf-002', 'user-003', 'علاقه‌مندی‌ها', 'کتاب‌های مورد علاقه', '["book-002"]', 0);

-- ---- ۵. اعلان‌های نمونه ----
INSERT INTO Notifications (id, receiverUserId, title, message, relatedEntityId, type, state, createdAt, readAt)
VALUES
    ('notif-001', 'user-003', 'کتاب جدید', 'کتاب برنامه‌نویسی با Qt منتشر شد!', 'book-001', 1, 0, datetime('now'), NULL);
