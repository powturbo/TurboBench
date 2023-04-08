# encoding: utf-8
# pysap - Python library for crafting SAP's network protocols packets
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Author:
#   Martin Gallo (@martingalloar)
#   Code contributed by SecureAuth to the OWASP CBAS project
#

# Standard imports
import os
import math
# Custom imports
from rsec import RSECCipher
# External imports
from cryptography.exceptions import InvalidKey
from cryptography.hazmat.primitives.hmac import HMAC
from cryptography.hazmat.primitives.ciphers import Cipher
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives import constant_time, padding
from cryptography.hazmat.primitives.hashes import Hash, SHA256, MD5

# Standard imports
from binascii import hexlify
# External imports
from scapy.utils import hexdump


def dpapi_decrypt_blob(blob, entropy=None):
    """Decrypts a blob of data using DPAPI.

    :param blob: encrypted blob to decrypt
    :type blob: string

    :param entropy: additional entropy to use to decrypt
    :type entropy: string

    :return: decrypted blob
    :rtype: string

    :raise Exception: if the platform is not Windows or the decryption failed
    """

    if os.name != "nt":
        raise Exception("DP API functions are only available in Windows")

    blob = bytes(blob)
    entropy = bytes(entropy or "")

    from ctypes import windll, byref, cdll, Structure, POINTER, c_char, c_buffer
    from ctypes.wintypes import DWORD

    LocalFree = windll.kernel32.LocalFree
    memcpy = cdll.msvcrt.memcpy
    CryptUnprotectData = windll.crypt32.CryptUnprotectData
    CRYPTPROTECT_UI_FORBIDDEN = 0x01

    class DATA_BLOB(Structure):
        _fields_ = [('cbData', DWORD),
                    ('pbData', POINTER(c_char))
                    ]

    def parse_data(data):
        cbData = int(data.cbData)
        pbData = data.pbData
        buffer = c_buffer(cbData)
        memcpy(buffer, pbData, cbData)
        LocalFree(pbData)
        return buffer.raw

    buffer_in = c_buffer(blob, len(blob))
    buffer_entropy = c_buffer(entropy, len(entropy))
    blob_in = DATA_BLOB(len(blob), buffer_in)
    blob_entropy = DATA_BLOB(len(entropy), buffer_entropy)
    blob_out = DATA_BLOB()

    if CryptUnprotectData(byref(blob_in), None, byref(blob_entropy), None, None,
                          CRYPTPROTECT_UI_FORBIDDEN, byref(blob_out)):
        return parse_data(blob_out)

    raise Exception("Unable to decrypt blob")


class PKCS12_PBKDF1(object):
    def __init__(self, algorithm, length, salt, iterations, id, backend):
        self._algorithm = algorithm
        self._length = length
        if not isinstance(salt, bytes):
            raise TypeError("salt must be bytes.")
        self._salt = salt
        self._iterations = iterations
        self._id = id
        self._backend = backend

    def derive(self, password):
        if not isinstance(password, bytes):
            raise TypeError("key_material must be bytes.")

        # Convert the password to UNICODE
        password = password.decode('utf-8').encode("utf-16be") + b'\x00\x00'

        u = self._algorithm.digest_size
        v = self._algorithm.block_size

        # Step 1 - Concatenate v/8 copies of ID
        d = chr(self._id) * v

        def concatenate_string(inp):
            s = b''
            if inp != b'':
                s_len = v * int(math.ceil(float(len(inp)) / v))
                while len(s) < s_len:
                    s += inp
                s = s[0:s_len]
            return s

        # Step 2 - Concatenate copies of the salt
        s = concatenate_string(self._salt)

        # Step 3 - Concatenate copies of the password
        p = concatenate_string(password)

        # Step 4 - Set I=S||P to be the concatenation of S and P
        i = s + p

        # Step 5 - Set c=ceiling(n/u)
        c = int(math.ceil(float(self._length) / u))

        # Step 6

        def digest(inp):
            h = Hash(self._algorithm(), backend=self._backend)
            h.update(inp)
            return h.finalize()

        def to_int(value):
            if value == b'':
                return 0
            return long(value.encode("hex"), 16)

        def to_bytes(value):
            value = "%x" % value
            if len(value) & 1:
                value = "0" + value
            return value.decode("hex")

        a = b'\x00' * (c * u)
        for n in range(1, c + 1):

            a2 = digest(d + i)
            for _ in range(2, self._iterations + 1):
                a2 = digest(a2)

            if n < c:
                # Step 6.B
                b = b''
                while len(b) < v:
                    b += a2

                b = to_int(b[:v]) + 1

                # Step 6.C
                for n2 in range(0, len(i) // v):
                    start = n2 * v
                    end = (n2 + 1) * v
                    i_n2 = i[start:end]
                    i_n2 = to_bytes(to_int(i_n2) + b)

                    i_n2_l = len(i_n2)
                    if i_n2_l > v:
                        i_n2 = i_n2[i_n2_l - v:]

                    i = i[0:start] + i_n2 + i[end:]

            # Step 7
            start = (n - 1) * u
            piece = min(self._length, u)
            a = a[:start] + a2[:piece] + a[start + piece:]

        return a[:self._length]

    def verify(self, key_material, expected_key):
        derived_key = self.derive(key_material)
        if not constant_time.bytes_eq(derived_key, expected_key):
            raise InvalidKey("Keys do not match.")


class PBKDF1(object):
    def __init__(self, algorithm, length, salt, iterations, backend):
        self._algorithm = algorithm
        self._length = length
        if not isinstance(salt, bytes):
            raise TypeError("salt must be bytes.")
        self._salt = salt
        self._iterations = iterations
        self._backend = backend

    def derive(self, key_material):
        if not isinstance(key_material, bytes):
            raise TypeError("key_material must be bytes.")
        h = Hash(self._algorithm(), backend=self._backend)
        h.update(key_material)
        h.update(self._salt)
        derived_key = h.finalize()
        for i in xrange(self._iterations-1):
            h = Hash(self._algorithm(), backend=self._backend)
            h.update(derived_key)
            derived_key = h.finalize()
        return derived_key

    def verify(self, key_material, expected_key):
        derived_key = self.derive(key_material)
        if not constant_time.bytes_eq(derived_key, expected_key):
            raise InvalidKey("Keys do not match.")


class PKCS12_PBES1(object):

    def __init__(self, salt, iterations, iv, password, hash_algorithm, enc_algorithm, enc_mode, backend):
        self._hash_algorithm = hash_algorithm
        self._enc_algorithm = enc_algorithm
        self._enc_mode = enc_mode
        self._backend = backend
        self._derive_key, self._iv = self.derive_key(salt, iterations, password)

    def derive_key(self, salt, iterations, password):
        pkcs12_pbkdf1 = PKCS12_PBKDF1(self._hash_algorithm, 24, salt, iterations, 1, self._backend)
        key = pkcs12_pbkdf1.derive(password)

        pkcs12_pbkdf1 = PKCS12_PBKDF1(self._hash_algorithm, 8, salt, iterations, 2, self._backend)
        iv = pkcs12_pbkdf1.derive(password)

        return key, iv

    def encrypt(self, plain_text):
        padder = padding.PKCS7(self._hash_algorithm.block_size).padder()
        plain_text = padder.update(plain_text) + padder.finalize()

        encryptor = Cipher(self._enc_algorithm(self._derive_key), self._enc_mode(self._iv), backend=self._backend).encryptor()
        cipher_text = encryptor.update(plain_text) + encryptor.finalize()

        return cipher_text

    def decrypt(self, cipher_text):
        padder = padding.PKCS7(self._hash_algorithm.block_size).padder()
        cipher_text = padder.update(cipher_text) + padder.finalize()

        decryptor = Cipher(self._enc_algorithm(self._derive_key), self._enc_mode(self._iv), backend=self._backend).decryptor()
        plain_text = decryptor.update(cipher_text) + decryptor.finalize()

        return plain_text


class SCRAM(object):
    """Base interface for implementing SCRAM password schemes.
    """

    CLIENT_PROOF_SIZE = 32
    CLIENT_KEY_SIZE = 64
    ALGORITHM = None

    def __init__(self, backend):
        """Initializes the SCRAM scheme"""
        self.backend = backend

    def get_client_key(self):
        """Returns a client key to be used during the handshake.
        """
        return os.urandom(self.CLIENT_KEY_SIZE)

    def salt_key(self, password, salt, rounds):
        hmac = HMAC(password, self.ALGORITHM(), self.backend)
        hmac.update(salt)
        return hmac.finalize()

    def scramble_salt(self, password, salt, server_key, client_key, rounds=None):
        """Scrambles a given salt using the specified server key.
        """
        msg = salt + server_key + client_key

        hmac_digest = self.salt_key(password, salt, rounds)

        hash = Hash(self.ALGORITHM(), self.backend)
        hash.update(hmac_digest)
        hash_digest = hash.finalize()

        key_hash = Hash(self.ALGORITHM(), self.backend)
        key_hash.update(hash_digest)
        key_hash_digest = key_hash.finalize()

        sig = HMAC(key_hash_digest, self.ALGORITHM(), self.backend)
        sig.update(msg)
        sig_digest = sig.finalize()

        return self.xor(sig_digest, hash_digest)

    def xor(self, a, b):
        """XOR two strings"""
        a = bytearray(a)
        b = bytearray(b)
        result = bytearray(len(a))
        for i in range(len(a)):
            result[i] += a[i] ^ b[i]
        return bytes(result)


class SCRAM_SHA256(SCRAM):
    """SCRAM scheme using SHA256 as the hashing algorithm.
    """
    ALGORITHM = SHA256


class SCRAM_MD5(SCRAM):
    """SCRAM scheme using MD5 as the hashing algorithm.
    """

    ALGORITHM = MD5


class SCRAM_PBKDF2SHA256(SCRAM_SHA256):
    """SCRAM scheme using PBKDF2 with SHA256"""

    def salt_key(self, password, salt, rounds):
        pbkdf2 = PBKDF2HMAC(self.ALGORITHM(), self.CLIENT_PROOF_SIZE, salt, rounds, self.backend)
        return pbkdf2.derive(password)


def rsec_decrypt(blob, key):
    """Decrypts a blob of data using SAP's RSEC decryption algorithm. The algorithm is based on
    the TripleDES.

    The decryption method is used in SSFS but also as part of other encryption schemes (e.g. RSECTAB),
    hence implemented in the crypto library instead of the particular layer.

    :param blob: encrypted blob to decrypt
    :type blob: bytes

    :param key: key to use to decrypt
    :type key: bytes

    :return: decrypted blob
    :rtype: bytes

    :raise Exception: if decryption failed
    """
    if len(key) != 24:
        raise Exception("Wrong key length")

    blob = [ord(i) for i in blob]
    key = [ord(i) for i in key]
    key1 = key[0:8]
    key2 = key[8:16]
    key3 = key[16:24]

    cipher = RSECCipher()
    round_1 = cipher.crypt(RSECCipher.MODE_DECODE, blob, key3, len(blob))
    round_2 = cipher.crypt(RSECCipher.MODE_ENCODE, round_1, key2, len(round_1))
    round_3 = cipher.crypt(RSECCipher.MODE_DECODE, round_2, key1, len(round_2))

    return ''.join([chr(i) for i in round_3])
