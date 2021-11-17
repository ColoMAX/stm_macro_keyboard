# with help from https://stackoverflow.com/a/2490718
from os import error
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.backends import default_backend
from cryptography.fernet import Fernet
from cryptography.exceptions import InvalidSignature
from cryptography.fernet import InvalidToken
from base64 import urlsafe_b64encode as b64e, urlsafe_b64decode as b64d
import secrets


Import("env")

# access to global construction environment
# print(env)

# Dump construction environment (for debug purpose)
# print(env.Dump())


backend = default_backend()
iterations = 100_000


def _derive_key(password: bytes, salt: bytes, iterations: int = iterations) -> bytes:
    """Derive a secret key from a given password and salt"""
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(), length=32, salt=salt,
        iterations=iterations, backend=backend)
    return b64e(kdf.derive(password))


def password_encrypt(message: bytes, password: str, iterations: int = iterations) -> bytes:
    salt = secrets.token_bytes(16)
    key = _derive_key(password.encode(), salt, iterations)
    return b64e(
        b'%b%b%b' % (
            salt,
            iterations.to_bytes(4, 'big'),
            b64d(Fernet(key).encrypt(message)),
        )
    )


def password_decrypt(token: bytes, password: str) -> bytes:
    decoded = b64d(token)
    salt, iter, token = decoded[:16], decoded[16:20], b64e(decoded[20:])
    iterations = int.from_bytes(iter, 'big')
    key = _derive_key(password.encode(), salt, iterations)
    try:    
        k = Fernet(key)
        d = k.decrypt(token)
    except (InvalidSignature, InvalidToken) as invdpsk:
        print("Wrong password")
        exit(0)
    return d

def before_build_macros(source, target, env):

    default_filename = "macros.ssb"

    class bcolors:
        HEADER = '\033[95m'
        OKBLUE = '\033[94m'
        OKCYAN = '\033[96m'
        OKGREEN = '\033[92m'
        WARNING = '\033[93m'
        FAIL = '\033[91m'
        ENDC = '\033[0m'
        BOLD = '\033[1m'
        UNDERLINE = '\033[4m'

    def cprint(string):
        print(bcolors.OKCYAN + string+bcolors.ENDC)

    # with open("src/macros_example.hpp", "r") as fp:
    #     countx = 0
    #     conty = 0
    #         for line in fp.readlines():
    #             if line.startswith("const byte ROWS"):
    #                  countx = line.
    #             elif line.startswith("const byte COLS"):
    #                 pass
    #             elif line.startswith("const char hexaKeys[ROWS][COLS]"):
    #                 bits = line.split(",")

    #                 print "Found array starting %s"%bits[0]
    #                 print "containing %d bytes"%count
    #                 print "in file %s"%header

    keys = list(range(1, 4))+['A']\
        + list(range(4, 7)) + ['B']\
        + list(range(7, 10))+['C']\
        + list('*0#D')

    cprint(
        'Welcome, this is the more secure macro input panel.\n')
    cprint(
        'Please provide c-synthax for the strings (thus if e.g. enter key needs to be pressed, use \\n).\n'
    )
    cprint(
        "If you wish to use the macro's specified in the header file, \
        disable this script in the platform.ini file."
    )
    cprint(
        'or type \"n\" , otherwise type anything else: \t')

    if(input() != 'n'):

        print('The assumed kayboard layout is 4x4\n')

        # keyboard layout printer
        for c, i in zip(keys, range(1, len(keys)+1)):

            d = ""

            if(i == 1):
                d += "______________\n"

            d += str(c)
            if(i % 4 != 0):
                d += " | "
            if(i % 4 == 0):
                d += '\n'+"______________\n"

            print(d, end='')

        print()
        print("read macros from file?[Y/n]")
        read = input()
        read = read != "n"

        if(read):
            import sys
            import os
            from getpass import getpass
            import pickle as p

            filenme = input(
                "Give filename to read macros from. Or press enter for default.\n filepath and name: ")
            if(filenme == ""):
                filenme = default_filename
            if not os.path.isfile(filenme):
                if(default_filename == filenme):
                    print(
                        bcolors.FAIL+" [Error] Standart file dos not excist yet"+bcolors.ENDC)
                else:
                    print(
                        bcolors.FAIL+" [Error] Please Specify Valid Input File"+bcolors.ENDC)
                exit(0)
            print("give the password for the file [correctness will not be checked]", flush=True)
            password = getpass()

            with open(filenme, "rb") as file:
                encrypted_data = file.read()

            value = password_decrypt(encrypted_data, password)
            value = p.loads(value)

        else:
            # read macro's from stdin
            value = []
            for i in keys:
                value.append(
                    input(f'Please enter macro for the \"{i}\"-key:\n'))

        # write back the macros
        for m, k in zip(value, keys):
            print(f'{k} saved macro = {m}')

        if(not read):
            # optionally save them
            print("save copy of macro's on host? [Y/n]")
            save = input()
            if(save != "n"):
                import sys
                import os
                from getpass import getpass
                import pickle as p
                print(
                    "Give filename to save macros on host. Or press enter for default.\n filepath and name: ")
                filenme = input()
                if(filenme == ""):
                    filenme = default_filename
                # if not os.path.isdir(filenme):
                #     print(bcolors.FAIL+" [Error] Please Specify Valid Output Path"+bcolors.ENDC)
                #     exit(0)

                with open(filenme, "wb") as file:
                    valid_psk = False
                    while(not valid_psk):
                        print("specify a password for the file", flush=True)
                        
                        password = getpass('Password:')
                        valid_psk = password == getpass('Again:')

                    encryptedData = password_encrypt(p.dumps(value), password)
                    file.write(encryptedData)
                print("Saved macros in encrypted file: "+filenme)
        # generate macros to build-system enviroment
        CPPDEFS = ["USE_SCRIPT_MACROS "]

        for m, k in zip(value, keys):
            if(k == '#'):
                name = "MACRO_H_NAME"
            elif(k == '*'):
                name = "MACRO_S_NAME"
            else:
                name = f"MACRO_{k}_NAME"
            if type(m) is None:
                m = ""
            CPPDEFS.append((name, m))

        # add genreated macros to build-system enviroment
        env.Append(CPPDEFINES=CPPDEFS)

        # print(env.Dump())


env.AddPreAction("${BUILD_DIR}/src/main.cpp.o", before_build_macros)
