-- =============================================================================
--  BookClub Database Schema
-- =============================================================================
--  SQLite schema for the BookClub server. Loaded by DatabaseManager::initialize().
-- =============================================================================

-- Users table
CREATE TABLE IF NOT EXISTS users (
    id TEXT PRIMARY KEY,
    username TEXT UNIQUE NOT NULL,
    display_name TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    security_question TEXT,
    security_answer_hash TEXT,
    role TEXT DEFAULT 'user' CHECK(role IN ('user', 'publisher', 'admin', 'server')),
    status TEXT DEFAULT 'active' CHECK(status IN ('active', 'blocked', 'inactive')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Genres table
CREATE TABLE IF NOT EXISTS genres (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT UNIQUE NOT NULL
);

-- Books table
CREATE TABLE IF NOT EXISTS books (
    id TEXT PRIMARY KEY,
    title TEXT NOT NULL,
    author_name TEXT,
    publisher_id TEXT,
    publisher_name TEXT,
    description TEXT,
    cover_color TEXT DEFAULT '#1A73E8',
    cover_accent TEXT DEFAULT '#1557B0',
    pdf_file_path TEXT,
    base_price REAL DEFAULT 0,
    price REAL DEFAULT 0,
    discount_value REAL DEFAULT 0,
    average_rating REAL DEFAULT 0,
    rating_count INTEGER DEFAULT 0,
    total_sales INTEGER DEFAULT 0,
    stock_count INTEGER DEFAULT 1,
    is_free INTEGER DEFAULT 0,
    active INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (publisher_id) REFERENCES users(id)
);

-- Book-genre junction table
CREATE TABLE IF NOT EXISTS book_genres (
    book_id TEXT NOT NULL,
    genre TEXT NOT NULL,
    PRIMARY KEY (book_id, genre),
    FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE CASCADE
);

-- Reviews table
CREATE TABLE IF NOT EXISTS reviews (
    id TEXT PRIMARY KEY,
    book_id TEXT NOT NULL,
    user_id TEXT NOT NULL,
    username TEXT NOT NULL,
    display_name TEXT,
    rating INTEGER NOT NULL CHECK(rating >= 1 AND rating <= 5),
    comment TEXT,
    helpful_count INTEGER DEFAULT 0,
    not_helpful_count INTEGER DEFAULT 0,
    current_user_helpful INTEGER DEFAULT 0,
    current_user_not_helpful INTEGER DEFAULT 0,
    verified_purchase INTEGER DEFAULT 0,
    by_author INTEGER DEFAULT 0,
    by_publisher INTEGER DEFAULT 0,
    pinned INTEGER DEFAULT 0,
    flagged INTEGER DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Review replies table
CREATE TABLE IF NOT EXISTS review_replies (
    id TEXT PRIMARY KEY,
    review_id TEXT NOT NULL,
    user_id TEXT,
    username TEXT,
    display_name TEXT,
    comment TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (review_id) REFERENCES reviews(id) ON DELETE CASCADE
);

-- Orders table
CREATE TABLE IF NOT EXISTS orders (
    id TEXT PRIMARY KEY,
    user_id TEXT NOT NULL,
    total REAL NOT NULL,
    discount REAL DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id)
);

-- Order items table
CREATE TABLE IF NOT EXISTS order_items (
    id TEXT PRIMARY KEY,
    order_id TEXT NOT NULL,
    book_id TEXT NOT NULL,
    price REAL NOT NULL,
    FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE CASCADE,
    FOREIGN KEY (book_id) REFERENCES books(id)
);

-- Notifications table
CREATE TABLE IF NOT EXISTS notifications (
    id TEXT PRIMARY KEY,
    user_id TEXT NOT NULL,
    category TEXT NOT NULL,
    title TEXT NOT NULL,
    body TEXT,
    icon TEXT,
    is_read INTEGER DEFAULT 0,
    is_archived INTEGER DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Shelves table
CREATE TABLE IF NOT EXISTS shelves (
    id TEXT PRIMARY KEY,
    user_id TEXT NOT NULL,
    name TEXT NOT NULL,
    description TEXT,
    color TEXT DEFAULT '#1A73E8',
    is_private INTEGER DEFAULT 0,
    is_favorite INTEGER DEFAULT 0,
    sort_order INTEGER DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Shelf-books junction table
CREATE TABLE IF NOT EXISTS shelf_books (
    shelf_id TEXT NOT NULL,
    book_id TEXT NOT NULL,
    PRIMARY KEY (shelf_id, book_id),
    FOREIGN KEY (shelf_id) REFERENCES shelves(id) ON DELETE CASCADE,
    FOREIGN KEY (book_id) REFERENCES books(id)
);

-- User library (purchased books)
CREATE TABLE IF NOT EXISTS user_library (
    user_id TEXT NOT NULL,
    book_id TEXT NOT NULL,
    purchased_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    is_downloaded INTEGER DEFAULT 0,
    PRIMARY KEY (user_id, book_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (book_id) REFERENCES books(id)
);

-- User wishlist
CREATE TABLE IF NOT EXISTS wishlist (
    user_id TEXT NOT NULL,
    book_id TEXT NOT NULL,
    added_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, book_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (book_id) REFERENCES books(id)
);

-- Discounts table
CREATE TABLE IF NOT EXISTS discounts (
    id TEXT PRIMARY KEY,
    book_id TEXT NOT NULL,
    publisher_id TEXT,
    discount_percent REAL NOT NULL,
    starts_at DATETIME,
    ends_at DATETIME,
    is_active INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE CASCADE
);

-- Promo codes table
CREATE TABLE IF NOT EXISTS promo_codes (
    code TEXT PRIMARY KEY,
    description TEXT,
    publisher_id TEXT,
    discount_percent INTEGER NOT NULL,
    scope TEXT DEFAULT 'catalog',
    max_uses INTEGER DEFAULT 1000,
    used_count INTEGER DEFAULT 0,
    starts_at DATETIME,
    ends_at DATETIME,
    is_active INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Study sessions table
CREATE TABLE IF NOT EXISTS study_sessions (
    id TEXT PRIMARY KEY,
    room_name TEXT NOT NULL,
    book_id TEXT NOT NULL,
    book_title TEXT,
    host_username TEXT NOT NULL,
    current_page INTEGER DEFAULT 1,
    page_count INTEGER DEFAULT 100,
    capacity INTEGER DEFAULT 10,
    is_private INTEGER DEFAULT 0,
    is_live INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (book_id) REFERENCES books(id)
);

-- Study session participants
CREATE TABLE IF NOT EXISTS study_session_participants (
    session_id TEXT NOT NULL,
    username TEXT NOT NULL,
    display_name TEXT,
    current_page INTEGER DEFAULT 1,
    joined_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (session_id, username),
    FOREIGN KEY (session_id) REFERENCES study_sessions(id) ON DELETE CASCADE
);

-- Study session chat messages
CREATE TABLE IF NOT EXISTS study_session_messages (
    id TEXT PRIMARY KEY,
    session_id TEXT NOT NULL,
    username TEXT NOT NULL,
    display_name TEXT,
    message TEXT NOT NULL,
    is_self INTEGER DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (session_id) REFERENCES study_sessions(id) ON DELETE CASCADE
);

-- Audit log
CREATE TABLE IF NOT EXISTS audit_log (
    id TEXT PRIMARY KEY,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    action TEXT NOT NULL,
    admin_username TEXT,
    target TEXT,
    details TEXT,
    severity TEXT DEFAULT 'info'
);

-- Reading progress (last page per book per user)
CREATE TABLE IF NOT EXISTS reading_progress (
    user_id TEXT NOT NULL,
    book_id TEXT NOT NULL,
    page INTEGER DEFAULT 1,
    page_count INTEGER DEFAULT 100,
    last_opened DATETIME DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, book_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (book_id) REFERENCES books(id)
);

-- Indexes for performance
CREATE INDEX IF NOT EXISTS idx_books_title ON books(title);
CREATE INDEX IF NOT EXISTS idx_books_author ON books(author_name);
CREATE INDEX IF NOT EXISTS idx_books_publisher ON books(publisher_id);
CREATE INDEX IF NOT EXISTS idx_books_active ON books(active);
CREATE INDEX IF NOT EXISTS idx_reviews_book ON reviews(book_id);
CREATE INDEX IF NOT EXISTS idx_reviews_user ON reviews(user_id);
CREATE INDEX IF NOT EXISTS idx_notifications_user ON notifications(user_id);
CREATE INDEX IF NOT EXISTS idx_notifications_read ON notifications(is_read);
CREATE INDEX IF NOT EXISTS idx_orders_user ON orders(user_id);
CREATE INDEX IF NOT EXISTS idx_shelves_user ON shelves(user_id);
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_status ON users(status);
