-- ============================================================
-- BookCLUB — SQLite production schema
-- ============================================================
-- Entities:
--   Roles, Users, Authors, Genres, Books, BookGenres,
--   Carts, CartItems, Orders, OrderItems,
--   Libraries, Shelves, ShelfBooks,
--   Reviews, Ratings,
--   Discounts, Notifications,
--   StudySessions, StudySessionParticipants,
--   Sessions (auth tokens)
-- ============================================================

PRAGMA foreign_keys = ON;

-- ============================================================
-- 1. Roles — lookup table for the three account roles
-- ============================================================
CREATE TABLE IF NOT EXISTS Roles (
    id    INTEGER PRIMARY KEY,           -- 0=User, 1=Publisher, 2=Admin (matches AccountRole enum)
    name  TEXT    UNIQUE NOT NULL
);
INSERT OR IGNORE INTO Roles (id, name) VALUES (0, 'user');
INSERT OR IGNORE INTO Roles (id, name) VALUES (1, 'publisher');
INSERT OR IGNORE INTO Roles (id, name) VALUES (2, 'admin');

-- ============================================================
-- 2. Users — base account table for all roles
-- ============================================================
CREATE TABLE IF NOT EXISTS Users (
    id                   TEXT PRIMARY KEY,             -- UUID
    username             TEXT    UNIQUE NOT NULL COLLATE NOCASE,
    passwordHash         TEXT    NOT NULL,             -- "salt$sha256(salt+pw)"
    displayName          TEXT    NOT NULL,
    email                TEXT    NOT NULL UNIQUE,
    phone                TEXT,
    securityQuestion     TEXT    NOT NULL,
    securityAnswerHash   TEXT    NOT NULL,
    status               INTEGER NOT NULL DEFAULT 1
                         CHECK (status IN (0,1,2,3,4)), -- 0=Pending,1=Active,2=Blocked,3=Disabled,4=Deleted
    role                 INTEGER NOT NULL DEFAULT 0
                         CHECK (role IN (0,1,2))
                         REFERENCES Roles(id),
    favoriteGenreIds     TEXT,                          -- JSON array (only for regular users)
    createdAt            DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updatedAt            DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- 3. Publishers — extended profile for role=1 users
-- ============================================================
CREATE TABLE IF NOT EXISTS Publishers (
    userId        TEXT    PRIMARY KEY REFERENCES Users(id) ON DELETE CASCADE,
    publisherName TEXT    NOT NULL,
    biography     TEXT,
    website       TEXT,
    taxId         TEXT,
    approved      INTEGER NOT NULL DEFAULT 1,           -- 0=pending approval, 1=approved
    createdAt     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- 4. Authors — normalised author table
-- ============================================================
CREATE TABLE IF NOT EXISTS Authors (
    id          TEXT PRIMARY KEY,
    name        TEXT NOT NULL,
    biography   TEXT,
    createdAt   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- 5. Genres — normalised genre table
-- ============================================================
CREATE TABLE IF NOT EXISTS Genres (
    id      TEXT PRIMARY KEY,
    name    TEXT UNIQUE NOT NULL COLLATE NOCASE,
    aliases TEXT                                           -- JSON array of alternative names
);

-- ============================================================
-- 6. Books
-- ============================================================
CREATE TABLE IF NOT EXISTS Books (
    id              TEXT PRIMARY KEY,
    title           TEXT    NOT NULL,
    authorId        TEXT    REFERENCES Authors(id) ON DELETE SET NULL,
    authorName      TEXT    NOT NULL,                       -- denormalised for fast search
    publisherId     TEXT    NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    description     TEXT,
    coverImagePath  TEXT,
    coverColor      TEXT    DEFAULT '#1A73E8',
    coverAccent     TEXT    DEFAULT '#1557B0',
    pdfFilePath     TEXT,
    basePrice       REAL    NOT NULL DEFAULT 0.0 CHECK (basePrice >= 0),
    discountValue   REAL    NOT NULL DEFAULT 0.0 CHECK (discountValue >= 0),
    averageRating   REAL    NOT NULL DEFAULT 0.0,
    ratingCount     INTEGER NOT NULL DEFAULT 0,
    totalSales      INTEGER NOT NULL DEFAULT 0,
    stockCount      INTEGER NOT NULL DEFAULT 0,
    isActive        INTEGER NOT NULL DEFAULT 1,
    visibility      INTEGER NOT NULL DEFAULT 0 CHECK (visibility IN (0,1,2)),  -- 0=Visible,1=Hidden,2=Archived
    availability    INTEGER NOT NULL DEFAULT 0 CHECK (availability IN (0,1,2)),-- 0=Available,1=Unavailable,2=Removed
    createdAt       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updatedAt       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- 7. BookGenres — many-to-many between Books and Genres
-- ============================================================
CREATE TABLE IF NOT EXISTS BookGenres (
    bookId  TEXT NOT NULL REFERENCES Books(id) ON DELETE CASCADE,
    genreId TEXT NOT NULL REFERENCES Genres(id) ON DELETE CASCADE,
    PRIMARY KEY (bookId, genreId)
);

-- ============================================================
-- 8. Carts — one-per-user, persistent across sessions
-- ============================================================
CREATE TABLE IF NOT EXISTS Carts (
    userId   TEXT PRIMARY KEY REFERENCES Users(id) ON DELETE CASCADE,
    updatedAt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- 9. CartItems
-- ============================================================
CREATE TABLE IF NOT EXISTS CartItems (
    id        TEXT PRIMARY KEY,
    userId    TEXT    NOT NULL REFERENCES Carts(userId) ON DELETE CASCADE,
    bookId    TEXT    NOT NULL REFERENCES Books(id) ON DELETE CASCADE,
    quantity  INTEGER NOT NULL DEFAULT 1 CHECK (quantity > 0),
    addedAt   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (userId, bookId)
);

-- ============================================================
-- 10. Orders
-- ============================================================
CREATE TABLE IF NOT EXISTS Orders (
    id            TEXT PRIMARY KEY,
    userId        TEXT    NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    subtotal      REAL    NOT NULL DEFAULT 0.0,
    discountTotal REAL    NOT NULL DEFAULT 0.0,
    finalTotal    REAL    NOT NULL DEFAULT 0.0,
    paid          INTEGER NOT NULL DEFAULT 0,
    completed     INTEGER NOT NULL DEFAULT 0,
    createdAt     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- 11. OrderItems
-- ============================================================
CREATE TABLE IF NOT EXISTS OrderItems (
    id          TEXT PRIMARY KEY,
    orderId     TEXT    NOT NULL REFERENCES Orders(id) ON DELETE CASCADE,
    bookId      TEXT    NOT NULL REFERENCES Books(id) ON DELETE RESTRICT,
    bookTitle   TEXT    NOT NULL,                              -- snapshot at purchase time
    unitPrice   REAL    NOT NULL,
    discountAmount REAL NOT NULL DEFAULT 0.0,
    quantity    INTEGER NOT NULL DEFAULT 1,
    lineTotal   REAL    NOT NULL
);

-- ============================================================
-- 12. Libraries — one-per-user (implicit in Users, kept for clarity)
-- ============================================================
CREATE TABLE IF NOT EXISTS Libraries (
    userId      TEXT PRIMARY KEY REFERENCES Users(id) ON DELETE CASCADE,
    lastOpenedBookId TEXT,
    lastOpenedPage   INTEGER DEFAULT 0,
    updatedAt   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- 13. Shelves — user-created collections inside the library
-- ============================================================
CREATE TABLE IF NOT EXISTS Shelves (
    id            TEXT PRIMARY KEY,
    userId        TEXT NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    name          TEXT NOT NULL,
    description   TEXT,
    color         TEXT DEFAULT '#1A73E8',
    isFavorite    INTEGER NOT NULL DEFAULT 0,
    isPrivate     INTEGER NOT NULL DEFAULT 0,
    isSystemShelf INTEGER NOT NULL DEFAULT 0,
    sortOrder     INTEGER NOT NULL DEFAULT 0,
    createdAt     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- 14. ShelfBooks — many-to-many
-- ============================================================
CREATE TABLE IF NOT EXISTS ShelfBooks (
    shelfId TEXT NOT NULL REFERENCES Shelves(id) ON DELETE CASCADE,
    bookId  TEXT NOT NULL REFERENCES Books(id)  ON DELETE CASCADE,
    addedAt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (shelfId, bookId)
);

-- ============================================================
-- 15. Reviews — text reviews (one per user per book)
-- ============================================================
CREATE TABLE IF NOT EXISTS Reviews (
    id              TEXT PRIMARY KEY,
    bookId          TEXT NOT NULL REFERENCES Books(id) ON DELETE CASCADE,
    userId          TEXT NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    userDisplayName TEXT NOT NULL,                             -- snapshot
    text            TEXT NOT NULL,
    stars           INTEGER NOT NULL DEFAULT 5 CHECK (stars BETWEEN 1 AND 5),
    isEdited        INTEGER NOT NULL DEFAULT 0,
    isPinned        INTEGER NOT NULL DEFAULT 0,
    isFlagged       INTEGER NOT NULL DEFAULT 0,
    helpfulCount    INTEGER NOT NULL DEFAULT 0,
    -- Issue 7: bump on each AddReviewReply. The reply text itself isn't
    -- persisted (no Replies table yet) — the counter is enough for the
    -- "N replies" badge in the client's ReviewItem.
    replyCount      INTEGER NOT NULL DEFAULT 0,
    createdAt       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updatedAt       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (bookId, userId)                                    -- one review per user per book
);

-- ============================================================
-- 16. Ratings — separate star-only rating (lightweight, allows
--     multiple ratings from same user over time)
-- ============================================================
CREATE TABLE IF NOT EXISTS Ratings (
    id        TEXT PRIMARY KEY,
    bookId    TEXT NOT NULL REFERENCES Books(id) ON DELETE CASCADE,
    userId    TEXT NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    stars     INTEGER NOT NULL CHECK (stars BETWEEN 1 AND 5),
    createdAt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (bookId, userId)                                    -- one rating per user per book
);

-- ============================================================
-- 17. Discounts — time-boxed promotions on a book
-- ============================================================
CREATE TABLE IF NOT EXISTS Discounts (
    id        TEXT PRIMARY KEY,
    bookId    TEXT NOT NULL REFERENCES Books(id) ON DELETE CASCADE,
    type      INTEGER NOT NULL DEFAULT 0 CHECK (type IN (0,1)), -- 0=Percentage, 1=FixedAmount
    value     REAL    NOT NULL DEFAULT 0.0 CHECK (value >= 0.0),
    startsAt  DATETIME NOT NULL,
    endsAt    DATETIME NOT NULL,
    isActive  INTEGER NOT NULL DEFAULT 1,
    createdBy TEXT REFERENCES Users(id),
    createdAt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK (endsAt > startsAt)
);

-- ============================================================
-- 18. Notifications
-- ============================================================
CREATE TABLE IF NOT EXISTS Notifications (
    id               TEXT PRIMARY KEY,
    receiverUserId   TEXT NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    title            TEXT NOT NULL,
    message          TEXT NOT NULL,
    relatedEntityId  TEXT,
    type             INTEGER NOT NULL DEFAULT 0
                      CHECK (type IN (0,1,2,3,4,5,6)),   -- matches NotificationType enum
    state            INTEGER NOT NULL DEFAULT 0
                      CHECK (state IN (0,1,2)),           -- 0=Unread,1=Read,2=Archived
    createdAt        DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    readAt           DATETIME
);

-- ============================================================
-- 19. StudySessions — bonus: group reading
-- ============================================================
CREATE TABLE IF NOT EXISTS StudySessions (
    id              TEXT PRIMARY KEY,
    bookId          TEXT NOT NULL REFERENCES Books(id) ON DELETE CASCADE,
    hostUserId      TEXT NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    state           INTEGER NOT NULL DEFAULT 0 CHECK (state IN (0,1,2,3)),
    currentPage     INTEGER NOT NULL DEFAULT 0,
    zoomLevel       REAL    NOT NULL DEFAULT 1.0,
    createdAt       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    closedAt        DATETIME
);

-- ============================================================
-- 20. StudySessionParticipants
-- ============================================================
CREATE TABLE IF NOT EXISTS StudySessionParticipants (
    sessionId   TEXT NOT NULL REFERENCES StudySessions(id) ON DELETE CASCADE,
    userId      TEXT NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    joinedAt    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (sessionId, userId)
);

-- ============================================================
-- 21. DiscountCodes — promotional coupon codes applied at cart
--     level (independent of per-book timed Discounts).
-- ============================================================
CREATE TABLE IF NOT EXISTS DiscountCodes (
    id              TEXT PRIMARY KEY,
    code            TEXT    UNIQUE NOT NULL COLLATE NOCASE,  -- e.g. "SUMMER20"
    type            INTEGER NOT NULL DEFAULT 0 CHECK (type IN (0,1)), -- 0=Percentage, 1=FixedAmount
    value           REAL    NOT NULL DEFAULT 0.0 CHECK (value >= 0.0),
    minCartTotal    REAL    NOT NULL DEFAULT 0.0,            -- minimum subtotal to activate
    maxUses         INTEGER NOT NULL DEFAULT 0,              -- 0 = unlimited
    usedCount       INTEGER NOT NULL DEFAULT 0,
    startsAt        DATETIME NOT NULL,
    endsAt          DATETIME NOT NULL,
    isActive        INTEGER NOT NULL DEFAULT 1,
    createdBy       TEXT REFERENCES Users(id),
    createdAt       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK (endsAt > startsAt)
);

CREATE INDEX IF NOT EXISTS idx_discountcodes_code ON DiscountCodes(code);

-- ============================================================
-- 22. Sessions — auth tokens (for persistent login)
-- ============================================================
CREATE TABLE IF NOT EXISTS Sessions (
    id        TEXT PRIMARY KEY,
    userId    TEXT NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    token     TEXT NOT NULL UNIQUE,
    expiresAt DATETIME NOT NULL,
    createdAt DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- Indexes for query performance
-- ============================================================
CREATE INDEX IF NOT EXISTS idx_users_username     ON Users(username);
CREATE INDEX IF NOT EXISTS idx_users_email        ON Users(email);
CREATE INDEX IF NOT EXISTS idx_users_role         ON Users(role);
CREATE INDEX IF NOT EXISTS idx_users_status       ON Users(status);

CREATE INDEX IF NOT EXISTS idx_books_publisherId  ON Books(publisherId);
CREATE INDEX IF NOT EXISTS idx_books_title        ON Books(title);
CREATE INDEX IF NOT EXISTS idx_books_authorName   ON Books(authorName);
CREATE INDEX IF NOT EXISTS idx_books_isActive     ON Books(isActive);
CREATE INDEX IF NOT EXISTS idx_books_createdAt    ON Books(createdAt);
CREATE INDEX IF NOT EXISTS idx_books_totalSales   ON Books(totalSales);

CREATE INDEX IF NOT EXISTS idx_bookgenres_bookId  ON BookGenres(bookId);
CREATE INDEX IF NOT EXISTS idx_bookgenres_genreId ON BookGenres(genreId);

CREATE INDEX IF NOT EXISTS idx_cartitems_userId   ON CartItems(userId);
CREATE INDEX IF NOT EXISTS idx_cartitems_bookId   ON CartItems(bookId);

CREATE INDEX IF NOT EXISTS idx_orders_userId      ON Orders(userId);
CREATE INDEX IF NOT EXISTS idx_orders_createdAt   ON Orders(createdAt);
CREATE INDEX IF NOT EXISTS idx_orders_completed   ON Orders(completed);

CREATE INDEX IF NOT EXISTS idx_orderitems_bookId  ON OrderItems(bookId);
CREATE INDEX IF NOT EXISTS idx_orderitems_orderId ON OrderItems(orderId);

CREATE INDEX IF NOT EXISTS idx_reviews_bookId     ON Reviews(bookId);
CREATE INDEX IF NOT EXISTS idx_reviews_userId     ON Reviews(userId);

CREATE INDEX IF NOT EXISTS idx_ratings_bookId     ON Ratings(bookId);
CREATE INDEX IF NOT EXISTS idx_ratings_userId     ON Ratings(userId);

CREATE INDEX IF NOT EXISTS idx_notifications_receiver ON Notifications(receiverUserId);
CREATE INDEX IF NOT EXISTS idx_notifications_state    ON Notifications(state);
CREATE INDEX IF NOT EXISTS idx_notifications_type     ON Notifications(type);

CREATE INDEX IF NOT EXISTS idx_shelves_userId      ON Shelves(userId);
CREATE INDEX IF NOT EXISTS idx_shelfbooks_shelfId  ON ShelfBooks(shelfId);
CREATE INDEX IF NOT EXISTS idx_shelfbooks_bookId   ON ShelfBooks(bookId);

CREATE INDEX IF NOT EXISTS idx_discounts_bookId    ON Discounts(bookId);
CREATE INDEX IF NOT EXISTS idx_discounts_active    ON Discounts(isActive);

CREATE INDEX IF NOT EXISTS idx_studysessions_bookId   ON StudySessions(bookId);
CREATE INDEX IF NOT EXISTS idx_studysessions_host      ON StudySessions(hostUserId);
CREATE INDEX IF NOT EXISTS idx_ssp_sessionId           ON StudySessionParticipants(sessionId);
CREATE INDEX IF NOT EXISTS idx_ssp_userId              ON StudySessionParticipants(userId);

CREATE INDEX IF NOT EXISTS idx_sessions_userId     ON Sessions(userId);
CREATE INDEX IF NOT EXISTS idx_sessions_token      ON Sessions(token);
