#!/usr/bin/env python3
"""Generate the proper salt$hash password hashes that BookClub's
PasswordHasher::hash() would produce, so the seed data can actually log in.

PasswordHasher::hash (per common/Utils/PasswordHasher.cpp):
  salt  = 16 random alphanumeric chars
  hash  = sha256(salt + plainPassword).hex()
  return salt + "$" + hash
"""
import hashlib, secrets, string, sys

ALPHABET = string.ascii_letters + string.digits

def gen_salt(n=16):
    return ''.join(secrets.choice(ALPHABET) for _ in range(n))

def hash_pw(plain):
    salt = gen_salt()
    h = hashlib.sha256((salt + plain).encode('utf-8')).hexdigest()
    return f"{salt}${h}"

if __name__ == '__main__':
    for plain in sys.argv[1:]:
        print(f"{plain!r:20s} -> {hash_pw(plain)}")
