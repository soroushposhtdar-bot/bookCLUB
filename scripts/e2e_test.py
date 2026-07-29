#!/usr/bin/env python3
"""End-to-end smoke test for BookCLUB server.

Tests the following flow:
  1.  Login as admin       -> 200
  2.  Login as publisher1  -> 200
  3.  Login as amir        -> 200
  4.  Get home sections    -> 200 + book list
  5.  Search books         -> 200 + book list
  6.  Get book details     -> 200 + book JSON
  7.  Get books by ids     -> 200 + batch
  8.  Add to cart          -> 200
  9.  Get cart             -> 200
 10.  Checkout             -> 200 + orderId + purchasedBookIds
 11.  Get library          -> 200 + purchasedBooks includes bought book
 12.  Create shelf         -> 200 + new shelf id
 13.  Add book to shelf    -> 200
 14.  Get notifications    -> 200
 15.  Publisher analytics  -> 200 + KPIs
 16.  Admin user list      -> 200 + users
 17.  Admin permission     -> publisher cannot BlockUser (403)
 18.  Logout               -> 200
"""
import socket, struct, json, sys, time

HOST = '127.0.0.1'
PORT = 8080

# Command codes from common/Network/Protocol.h
CMD = {
    "Invalid": 0, "Login": 1, "Register": 2, "ResetPassword": 3, "ChangePassword": 4,
    "Logout": 5, "GetHomeSections": 6, "SearchBooks": 7, "GetBookDetails": 8,
    "GetBooksByIds": 9, "AddToCart": 10, "RemoveFromCart": 11, "GetCart": 12,
    "Checkout": 13, "ApplyDiscount": 14, "ClearCart": 15, "GetLibrary": 16,
    "GetPurchasedBooks": 17, "CreateShelf": 18, "DeleteShelf": 19, "AddBookToShelf": 20,
    "RemoveBookFromShelf": 21, "RenameShelf": 22, "SubmitReview": 23, "UpdateReview": 24,
    "DeleteReview": 25, "SetRating": 26, "ToggleWishlist": 27, "GetWishlist": 28,
    "GetPublisherBooks": 29, "PublishBook": 30, "UpdateBook": 31, "DeactivateBook": 32,
    "ActivateBook": 33, "ApplyTimedDiscount": 34, "GetPublisherAnalytics": 35,
    "GetUsersList": 36, "BlockUser": 37, "UnblockUser": 38, "DeleteUser": 39,
    "ModerateBook": 40, "RemoveBookByAdmin": 41, "GetNotifications": 42,
    "MarkNotificationRead": 43, "MarkAllNotificationsRead": 44,
    "CreateStudySession": 45, "JoinStudySession": 46, "LeaveStudySession": 47,
    "SyncStudyPage": 48, "GetServerHealth": 49, "GetServerLogs": 50,
    "GetServerClients": 51, "GetCurrentUser": 52, "UpdateProfile": 53,
    "SaveFavoriteGenres": 54, "GetGenres": 55, "GetAuthors": 56,
    "UploadBookPdf": 57, "DownloadBookPdf": 58, "UpdateShelf": 59,
    # Bug 2/3/4/5: new StudySession commands. Explicit values keep them
    # stable across future Protocol.h edits (see the comments in Protocol.h).
    "GetStudySessions": 114, "SendStudyMessage": 115,
    "InviteToStudySession": 116, "AddStudyNote": 117, "GetStudyNotes": 118,
    "EvtNotification": 1000, "EvtReviewUpdated": 1001, "EvtStudySync": 1002,
    "EvtBookAdded": 1003, "EvtDiscountApplied": 1004, "EvtUserBlocked": 1005,
    "EvtServerShutdown": 1006,
    # Bug 2/3/5/8: new server-pushed events.
    "EvtSessionListUpdated": 1007, "EvtStudyMessage": 1008,
    "EvtStudyNote": 1009, "EvtStudyParticipantChanged": 1010,
}

class Client:
    def __init__(self):
        self.sock = socket.socket()
        self.sock.settimeout(5.0)
        self.sock.connect((HOST, PORT))

    def send(self, cmd_name, payload=None):
        if payload is None: payload = {}
        msg = {
            "requestId": "req-" + str(time.time_ns()),
            "command": CMD[cmd_name],
            "status": 200,
            "payload": payload,
        }
        js = json.dumps(msg).encode('utf-8')
        self.sock.sendall(struct.pack('>I', len(js)) + js)
        return self.recv()

    def recv(self):
        hdr = b''
        while len(hdr) < 4:
            c = self.sock.recv(4 - len(hdr))
            if not c: return None
            hdr += c
        n = struct.unpack('>I', hdr)[0]
        body = b''
        while len(body) < n:
            c = self.sock.recv(n - len(body))
            if not c: break
            body += c
        return json.loads(body.decode('utf-8')) if body else None

    def close(self):
        self.sock.close()


def step(label, response, expect_status=200):
    status = response.get('status') if response else None
    ok = status == expect_status
    mark = 'OK ' if ok else 'FAIL'
    print(f"[{mark}] {label:35s}  status={status}")
    if not ok:
        print("       response:", json.dumps(response, ensure_ascii=False)[:200])
    return ok


def main():
    print("=== Test 1: Login as admin ===")
    admin = Client()
    r = admin.send("Login", {"username": "admin", "password": "admin"})
    if not step("admin login", r): return 1

    print("\n=== Test 2: Login as publisher1 ===")
    pub = Client()
    r = pub.send("Login", {"username": "publisher1", "password": "publisher1"})
    if not step("publisher1 login", r): return 1

    print("\n=== Test 3: Login as amir (regular user) ===")
    user = Client()
    r = user.send("Login", {"username": "amir", "password": "amir1234"})
    if not step("amir login", r): return 1

    print("\n=== Test 4: Get home sections ===")
    r = user.send("GetHomeSections")
    if step("GetHomeSections", r):
        sections = r['payload']
        for k in ('featured', 'newBooks', 'bestSellers', 'freeBooks'):
            print(f"       {k}: {len(sections.get(k, []))} books")

    print("\n=== Test 5: Search books ===")
    r = user.send("SearchBooks", {"keyword": "Qt"})
    step("SearchBooks 'Qt'", r)
    book_id = 'book-001'
    if r and r['payload'].get('results'):
        book_id = r['payload']['results'][0]['id']
        print(f"       First book id: {book_id}")

    print("\n=== Test 6: Get book details ===")
    r = user.send("GetBookDetails", {"bookId": book_id})
    step("GetBookDetails", r)
    if r and r['payload'].get('publisherName'):
        print(f"       publisherName: {r['payload']['publisherName']}")

    print("\n=== Test 7: Get books by ids (batch) ===")
    r = user.send("GetBooksByIds", {"bookIds": ["book-001", "book-002", "nonexistent"]})
    if step("GetBooksByIds", r):
        print(f"       returned {r['payload'].get('count')} books")

    print("\n=== Test 8: Add to cart ===")
    r = user.send("AddToCart", {"bookId": book_id, "quantity": 1})
    step("AddToCart", r)

    print("\n=== Test 9: Get cart ===")
    r = user.send("GetCart")
    if step("GetCart", r):
        print(f"       items: {r['payload'].get('itemCount')}, total: {r['payload'].get('total')}")

    print("\n=== Test 10: Checkout ===")
    r = user.send("Checkout")
    if step("Checkout", r):
        purchased = r['payload'].get('purchasedBookIds', [])
        print(f"       orderId: {r['payload'].get('orderId')}")
        print(f"       purchasedBookIds: {purchased}")

    print("\n=== Test 11: Get library ===")
    r = user.send("GetLibrary")
    if step("GetLibrary", r):
        purchased = r['payload'].get('purchasedBookIds', [])
        print(f"       purchased books: {purchased}")
        if book_id in purchased:
            print("       OK purchased book is in library")
        else:
            print(f"       FAIL expected {book_id} in {purchased}")

    print("\n=== Test 12: Create shelf + add book ===")
    r = user.send("CreateShelf", {"name": "test_favorites", "description": "test shelf"})
    if step("CreateShelf", r):
        shelf_id = r['payload'].get('newShelfId')
        print(f"       new shelf id: {shelf_id}")
        if shelf_id:
            r = user.send("AddBookToShelf", {"shelfId": shelf_id, "bookId": book_id})
            step("AddBookToShelf", r)

    print("\n=== Test 13: Get notifications ===")
    r = user.send("GetNotifications")
    if step("GetNotifications", r):
        print(f"       notifications: {r['payload'].get('count')}")

    print("\n=== Test 14: Publisher analytics ===")
    r = pub.send("GetPublisherAnalytics")
    if step("GetPublisherAnalytics", r):
        print(f"       totalBooks: {r['payload'].get('totalBooks')}, "
              f"totalSales: {r['payload'].get('totalSales')}, "
              f"totalRevenue: {r['payload'].get('totalRevenue')}")

    print("\n=== Test 15: Admin user list ===")
    r = admin.send("GetUsersList")
    if step("GetUsersList", r):
        print(f"       users: {r['payload'].get('count')}")

    print("\n=== Test 16: Publisher cannot call admin command (should 403) ===")
    r = pub.send("BlockUser", {"userId": "user-003"})
    step("Publisher BlockUser (expect 403)", r, expect_status=403)

    print("\n=== Test 17: Regular user cannot call publisher command (should 403) ===")
    r = user.send("PublishBook", {"title": "Test", "authorName": "Test", "basePrice": 0})
    step("User PublishBook (expect 403)", r, expect_status=403)

    print("\n=== Test 18: Logout ===")
    r = user.send("Logout")
    step("Logout", r)

    print("\n=== All tests completed ===")
    admin.close()
    pub.close()
    user.close()
    return 0


if __name__ == '__main__':
    sys.exit(main())
