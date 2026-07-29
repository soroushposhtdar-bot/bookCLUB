-- ============================================================
-- BookCLUB — minimal seed (admins only)
-- ============================================================
-- Credentials:
--   amir     / amir1234
--   soroush  / soroush
-- ============================================================

-- ---- Users ----
INSERT OR IGNORE INTO Users
    (id, username, passwordHash, displayName, email, phone,
     securityQuestion, securityAnswerHash,
     status, role, favoriteGenreIds, createdAt, updatedAt)
VALUES
    ('user-001', 'amir',
     'PE3elia3BGws2fhC$580c039784ae03d4ba830f1ccbc82bcd4ba3b0429c76dbbcc8465588b5656bff',
     'Amir Hosseini', 'amir@bookclub.com', '09123333333',
     'What was the title of your favourite childhood book?',
     'FD7KHsT0UfnKHivl$a29b77715696c2f7440d2f7846fa845085ec5726764bf2e84a53ace0700c9cee',
     1, 2, NULL, datetime('now'), datetime('now')),

    ('user-002', 'soroush',
     '9PRAmxH1jLeY4g1c$36b19584709b684aded5e570c704e1f94c28d2694480549849387f0c3769de7e',
     'Soroush Admin', 'soroush@bookclub.com', '09125555555',
     'What was the title of your favourite childhood book?',
     'K47aYU6RseKwsnUR$dcb10777514c2cb7e4a9d921b12863117b7c9fdfa93f6948ec2385a21a9642b2',
     1, 2, NULL, datetime('now'), datetime('now'));
