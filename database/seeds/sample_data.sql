-- ============================================================
-- BookCLUB — seed data
-- ============================================================
-- Passwords (salt$sha256):
--   admin       / admin
--   publisher1  / publisher1
--   amir        / amir1234
--   soroush     / soroush
--   server      / server
-- Security answers (lowercased + trimmed before hashing):
--   admin       -> "shahnameh"
--   publisher1  -> "saadi"
--   amir        -> "mollanasreddin"
-- ============================================================

-- ---- 1. Users ----
INSERT OR IGNORE INTO Users
    (id, username, passwordHash, displayName, email, phone, securityQuestion, securityAnswerHash, status, role, favoriteGenreIds, createdAt, updatedAt)
VALUES
    ('user-001', 'admin',
     'bqxGpZfaVjiKj9bx$51a395b183638c39eb5d02c191207e18f3531fd81d1e008d32194635e0f215cd',
     'System Administrator', 'admin@bookclub.com', '09121111111',
     'What was the title of your favourite childhood book?',
     'xeEgK4duaryKUt20$fea68b3065b76af3f4901d736ccdb62a8a07fdd302e03e595a034e74c37c4b6d',
     1, 2, NULL, datetime('now'), datetime('now')),

    ('user-002', 'publisher1',
     '0yzDxCZH5qcStwxl$9d27145021ad167bca1616ca02452b26a37ecf3441ce8fd439c7b5725aaead2d',
     'Saadi Publications', 'pub1@bookclub.com', '09122222222',
     'Who is your favourite author?',
     'gZhZkiPfnNT9a93Y$9159de614086f677ef9454847c28234c0d2429b4eb89b8c10790071f77aa83ae',
     1, 1, NULL, datetime('now'), datetime('now')),

    ('user-003', 'amir',
     'PE3elia3BGws2fhC$580c039784ae03d4ba830f1ccbc82bcd4ba3b0429c76dbbcc8465588b5656bff',
     'Amir Hosseini', 'amir@bookclub.com', '09123333333',
     'What was the title of your favourite childhood book?',
     'FD7KHsT0UfnKHivl$a29b77715696c2f7440d2f7846fa845085ec5726764bf2e84a53ace0700c9cee',
     1, 0, '["Programming","Novel"]', datetime('now'), datetime('now')),

    ('user-004', 'sara',
     'PE3elia3BGws2fhC$580c039784ae03d4ba830f1ccbc82bcd4ba3b0429c76dbbcc8465588b5656bff',
     'Sara Ahmadi', 'sara@bookclub.com', '09124444444',
     'In what city were you born?',
     'FD7KHsT0UfnKHivl$a29b77715696c2f7440d2f7846fa845085ec5726764bf2e84a53ace0700c9cee',
     1, 0, '["Novel","History"]', datetime('now'), datetime('now')),

    ('user-005', 'soroush',
     '9PRAmxH1jLeY4g1c$36b19584709b684aded5e570c704e1f94c28d2694480549849387f0c3769de7e',
     'Soroush Admin', 'soroush@bookclub.com', '09125555555',
     'What was the title of your favourite childhood book?',
     'K47aYU6RseKwsnUR$dcb10777514c2cb7e4a9d921b12863117b7c9fdfa93f6948ec2385a21a9642b2',
     1, 2, NULL, datetime('now'), datetime('now'));

    ('user-006', 'server',
     'h5qqqYvKT50JOfQY$5a783be762af88f1fe862ec71844b362ff549237dd67d5d3b086950e51b74be6',
     'Server Operator', 'server@bookclub.com', '09126666666',
     'What was the title of your favourite childhood book?',
     'GdTOAiaCeOqNcMda$2c3c485d3c2444c19945b4678edf781f18ee37c364cedc267a5cab2dc17d2091',
     1, 3, NULL, datetime('now'), datetime('now'));
-- ---- 2. Publishers (extended profile) ----
INSERT OR IGNORE INTO Publishers (userId, publisherName, biography, website, taxId, approved)
VALUES
    ('user-002', 'Saadi Publications',
     'Independent publisher specialising in Persian literature and programming books.',
     'https://saadi.example.com', 'TAX-002', 1);

-- ---- 3. Authors ----
INSERT OR IGNORE INTO Authors (id, name, biography) VALUES
    ('author-001', 'Saeed Mohammadi',   'Senior Qt developer and instructor.'),
    ('author-002', 'Maryam Ahmadi',     'C++ expert specialising in modern C++ and STL.'),
    ('author-003', 'Abolghasem Ferdowsi','Persian epic poet, author of the Shahnameh.'),
    ('author-004', 'Saadi Shirazi',     'Classical Persian poet.');

-- ---- 4. Genres ----
-- Issue 3: genre IDs now use the human-readable display name as the
-- primary key. This matches what the client expects in
-- AuthService::_defaultGenres() / BookService::availableGenres(),
-- so `book.genreIds` and `user.favoriteGenreIds` no longer leak raw
-- Genre IDs use real display names (no more genre-001).
INSERT OR IGNORE INTO Genres (id, name, aliases) VALUES
    ('Programming', 'Programming',  '["Software","Coding"]'),
    ('Novel',       'Novel',        '["Fiction","Romance"]'),
    ('History',     'History',      '["Historical"]'),
    ('Poetry',      'Poetry',       '[]'),
    ('Biography',   'Biography',    '[]'),
    ('Self-Help',   'Self-Help',    '[]'),
    ('Business',    'Business',     '[]'),
    ('Science',     'Science',      '["SciFi","Science Fiction"]');

-- ---- 5. Books ----
INSERT OR IGNORE INTO Books
    (id, title, authorId, authorName, publisherId, description, coverImagePath, coverColor, coverAccent,
     pdfFilePath, basePrice, discountValue, averageRating, ratingCount, totalSales, stockCount,
     isActive, visibility, availability, createdAt, updatedAt)
VALUES
    ('book-001', 'Programming with Qt', 'author-001', 'Saeed Mohammadi', 'user-002',
     'A complete guide to Qt 6 and QML for building cross-platform applications.',
     '/images/qt_book.jpg',  '#1A73E8', '#1557B0',
     '/pdfs/qt_book.pdf', 35.0, 5.0, 4.5, 10, 20, 50, 1, 0, 0, datetime('now'), datetime('now')),

    ('book-002', 'Secrets of C++', 'author-002', 'Maryam Ahmadi', 'user-002',
     'Advanced C++ concepts: templates, move semantics, smart pointers and more.',
     '/images/cpp_book.jpg', '#34A853', '#2E7D32',
     '/pdfs/cpp_book.pdf', 45.0, 0.0, 4.8, 8, 15, 30, 1, 0, 0, datetime('now'), datetime('now')),

    ('book-003', 'Shahnameh for Everyone', 'author-003', 'Abolghasem Ferdowsi', 'user-002',
     'A simple retelling of the stories of the Shahnameh.',
     '/images/shahnameh.jpg', '#FBBC05', '#F9A825',
     '/pdfs/shahnameh.pdf', 60.0, 0.0, 4.9, 25, 100, 200, 1, 0, 0, datetime('now'), datetime('now')),

    ('book-004', 'The Gulistan', 'author-004', 'Saadi Shirazi', 'user-002',
     'A classic of Persian literature, blending poetry and prose.',
     '/images/gulistan.jpg', '#EA4335', '#C62828',
     '/pdfs/gulistan.pdf', 25.0, 5.0, 4.7, 12, 35, 80, 1, 0, 0, datetime('now'), datetime('now')),

    ('book-005', 'Modern C++ in 30 Days', 'author-002', 'Maryam Ahmadi', 'user-002',
     'A hands-on introduction to modern C++ features.',
     '/images/cpp30.jpg', '#9C27B0', '#6A1B9A',
     '/pdfs/cpp30.pdf', 0.0, 0.0, 4.3, 5, 50, 100, 1, 0, 0, datetime('now'), datetime('now'));

-- ---- 6. BookGenres ----
-- Issue 3: genreId now stores the genre's display name (matches the
-- new `Genres.id` above).
INSERT OR IGNORE INTO BookGenres (bookId, genreId) VALUES
    ('book-001', 'Programming'),
    ('book-002', 'Programming'),
    ('book-003', 'History'),
    ('book-004', 'Poetry'),
    ('book-005', 'Programming');

-- ---- 7. Carts (one per user) ----
INSERT OR IGNORE INTO Carts (userId) VALUES
    ('user-003'),
    ('user-004');

-- ---- 8. Libraries ----
INSERT OR IGNORE INTO Libraries (userId, lastOpenedBookId, lastOpenedPage) VALUES
    ('user-003', 'book-001', 5),
    ('user-004', NULL, 0);

-- ---- 9. Shelves ----
INSERT OR IGNORE INTO Shelves (id, userId, name, description, color, isFavorite, isPrivate, isSystemShelf, sortOrder)
VALUES
    ('shelf-001', 'user-003', 'Read',        'Books I have finished', '#1A73E8', 1, 0, 1, 0),
    ('shelf-002', 'user-003', 'Wishlist',    'Books I want to read',  '#EA4335', 0, 0, 1, 1),
    ('shelf-003', 'user-003', 'Favourites',  'My personal favourites','#34A853', 1, 0, 0, 2),
    ('shelf-004', 'user-004', 'Read',        'Books I have finished', '#1A73E8', 1, 0, 1, 0);

-- ---- 10. ShelfBooks ----
INSERT OR IGNORE INTO ShelfBooks (shelfId, bookId) VALUES
    ('shelf-001', 'book-001'),
    ('shelf-003', 'book-002'),
    ('shelf-003', 'book-003');

-- ---- 11. Reviews ----
INSERT OR IGNORE INTO Reviews (id, bookId, userId, userDisplayName, text, stars, isEdited, createdAt, updatedAt)
VALUES
    ('rev-001', 'book-001', 'user-003', 'Amir Hosseini',
     'An excellent book! The examples are clear and the explanations are thorough.', 5, 0,
     datetime('now'), datetime('now')),
    ('rev-002', 'book-002', 'user-003', 'Amir Hosseini',
     'Very useful and well-written.', 4, 0, datetime('now'), datetime('now')),
    ('rev-003', 'book-003', 'user-004', 'Sara Ahmadi',
     'A beautiful retelling of the Shahnameh stories.', 5, 0,
     datetime('now'), datetime('now'));

-- ---- 12. Ratings (lightweight — separate from reviews) ----
INSERT OR IGNORE INTO Ratings (id, bookId, userId, stars) VALUES
    ('rat-001', 'book-001', 'user-004', 5),
    ('rat-002', 'book-002', 'user-004', 4),
    ('rat-003', 'book-003', 'user-003', 5);

-- ---- 13. Discounts (time-boxed promotion) ----
INSERT OR IGNORE INTO Discounts (id, bookId, type, value, startsAt, endsAt, isActive, createdBy)
VALUES
    ('disc-001', 'book-001', 0, 10.0,
     datetime('now', '-1 day'), datetime('now', '+7 day'), 1, 'user-002');

-- ---- 14. Notifications ----
INSERT OR IGNORE INTO Notifications
    (id, receiverUserId, title, message, relatedEntityId, type, state, createdAt, readAt)
VALUES
    ('notif-001', 'user-003', 'New book published',
     'A new book in your favourite genre has been published!',
     'book-001', 1, 0, datetime('now'), NULL),
    ('notif-002', 'user-002', 'New sale',
     'Your book "Programming with Qt" was just purchased!',
     'book-001', 3, 0, datetime('now'), NULL);

-- ---- 15. StudySessions ----
INSERT OR IGNORE INTO StudySessions (id, bookId, hostUserId, state, currentPage, zoomLevel, createdAt)
VALUES
    ('sess-001', 'book-001', 'user-003', 1, 3, 1.0, datetime('now'));

INSERT OR IGNORE INTO StudySessionParticipants (sessionId, userId) VALUES
    ('sess-001', 'user-003'),
    ('sess-001', 'user-004');
