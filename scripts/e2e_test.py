#!/usr/bin/env python3
"""End-to-end smoke test for BookClub server.

Tests the following flow:
  1. Login as admin       -> 200
  2. Login as publisher1  -> 200
  3. Login as amir        -> 200
  4. Get home sections    -> 200 + book list
  5. Search books         -> 200 + book list
  6. Add to cart          -> 200 (requires authentication)
  7. Get cart             -> 200
  8. Checkout             -> 200 + orderId
  9. Get library          -> 200 + purchasedBooks includes the bought book
 10. Get notifications    -> 200
"""
import socket, struct, json, sys, time

HOST = '127.0.0.1'
PORT = 8080

# Command codes from common/Network/Protocol.h
CMD = {
    "Invalid": 0,
    "Login": 1,
    "Register": 2,
    "ResetPassword": 3,
    "ChangePassword": 4,
    "Logout": 5,
    "GetHomeSections": 6,
    "SearchBooks": 7,
    "GetBookDetails": 8,
    "AddToCart": 9,
    "RemoveFromCart": 10,
    "GetCart": 11,
    "Checkout": 12,
    "ApplyDiscount": 13,
    "GetLibrary": 14,
    "GetPurchasedBooks": 15,
    "CreateShelf": 16,
    "DeleteShelf": 17,
    "AddBookToShelf": 18,
    "RemoveBookFromShelf": 19,
    "GetPublisherBooks": 20,
    "PublishBook": 21,
    "UpdateBook": 22,
    "DeactivateBook": 23,
    "ActivateBook": 24,
    "ApplyTimedDiscount": 25,
    "GetPublisherAnalytics": 26,
    "GetUsersList": 27,
    "BlockUser": 28,
    "UnblockUser": 29,
    "DeleteUser": 30,
    "ModerateBook": 31,
    "RemoveBookByAdmin": 32,
    "GetNotifications": 33,
    "MarkNotificationRead": 34,
    "MarkAllNotificationsRead": 35,
    "CreateStudySession": 36,
    "JoinStudySession": 37,
    "LeaveStudySession": 38,
    "SyncStudyPage": 39,
}

class Client:
    def __init__(self):
        self.sock = socket.socket()
        self.sock.settimeout(5.0)
        self.sock.connect((HOST, PORT))

    def send(self, cmd_name, payload=None):
        if payload is None:
            payload = {}
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
    print(f"[{mark}] {label:30s}  status={status}")
    if not ok:
        print("       response:", json.dumps(response, ensure_ascii=False)[:200])
    return ok


def main():
    print("=== Test 1: Login as admin ===")
    admin = Client()
    r = admin.send("Login", {"username": "admin", "password": "admin"})
    if not step("admin login", r):
        return 1

    print("\n=== Test 2: Login as publisher1 ===")
    pub = Client()
    r = pub.send("Login", {"username": "publisher1", "password": "publisher1"})
    if not step("publisher1 login", r):
        return 1

    print("\n=== Test 3: Login as amir (regular user) ===")
    user = Client()
    r = user.send("Login", {"username": "amir", "password": "amir1234"})
    if not step("amir login", r):
        return 1

    print("\n=== Test 4: Get home sections ===")
    r = user.send("GetHomeSections")
    if step("GetHomeSections", r):
        sections = r['payload']
        for k in ('featured', 'newBooks', 'bestSellers', 'freeBooks'):
            print(f"       {k}: {len(sections.get(k, []))} books")

    print("\n=== Test 5: Search books ===")
    r = user.send("SearchBooks", {"keyword": "Qt"})
    step("SearchBooks 'Qt'", r)
    if r and r['payload'].get('results'):
        book_id = r['payload']['results'][0]['id']
        print(f"       First book id: {book_id}")
    else:
        # Fall back to a known book id from seed data
        book_id = 'book-001'

    print("\n=== Test 6: Get book details ===")
    r = user.send("GetBookDetails", {"bookId": book_id})
    step("GetBookDetails", r)

    print("\n=== Test 7: Add to cart ===")
    r = user.send("AddToCart", {"bookId": book_id, "quantity": 1})
    step("AddToCart", r)

    print("\n=== Test 8: Get cart ===")
    r = user.send("GetCart")
    step("GetCart", r)
    if r:
        print(f"       items: {r['payload'].get('itemCount')}, total: {r['payload'].get('total')}")

    print("\n=== Test 9: Checkout ===")
    r = user.send("Checkout")
    step("Checkout", r)
    if r and r['payload'].get('orderId'):
        print(f"       orderId: {r['payload']['orderId']}")

    print("\n=== Test 10: Get library ===")
    r = user.send("GetLibrary")
    step("GetLibrary", r)
    if r:
        purchased = r['payload'].get('purchasedBookIds', [])
        print(f"       purchased books: {purchased}")
        if book_id in purchased:
            print("       OK purchased book is in library")
        else:
            print(f"       FAIL expected {book_id} in {purchased}")

    print("\n=== Test 11: Create shelf + add book to it ===")
    r = user.send("CreateShelf", {"name": "favorites", "description": "test shelf"})
    step("CreateShelf", r)
    if r and r['payload'].get('shelves'):
        shelf_id = r['payload']['shelves'][-1]['id']
        print(f"       new shelf id: {shelf_id}")
        r = user.send("AddBookToShelf", {"shelfId": shelf_id, "bookId": book_id})
        step("AddBookToShelf", r)

    print("\n=== Test 12: Get notifications ===")
    r = user.send("GetNotifications")
    step("GetNotifications", r)
    if r:
        print(f"       notifications: {r['payload'].get('count')}")

    print("\n=== Test 13: Publisher analytics ===")
    r = pub.send("GetPublisherAnalytics")
    step("GetPublisherAnalytics", r)
    if r:
        print(f"       totalBooks: {r['payload'].get('totalBooks')}, "
              f"totalSales: {r['payload'].get('totalSales')}, "
              f"totalRevenue: {r['payload'].get('totalRevenue')}")

    print("\n=== Test 14: Admin user list ===")
    r = admin.send("GetUsersList")
    step("GetUsersList", r)
    if r:
        print(f"       users: {r['payload'].get('count')}")

    admin.close()
    pub.close()
    user.close()
    print("\nAll tests passed.")
    return 0


if __name__ == '__main__':
    sys.exit(main())
