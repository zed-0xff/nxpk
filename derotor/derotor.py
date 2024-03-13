#!/usr/bin/env python2
import sys
import zlib

def init_rotor():
    asdf_tm = "zxp+3uexi,fjepkfap3341s~mnv+1we!/zxp+3uexi,fj$~ap3341sfjepkfap3341s+3uexifjepkfap3341exi,+3uex22i,+3t56uexi,+3ue34xi,+3u34exi,+tttuexi,+lzuexi,~mnv+1we!/zxp~mnv+1we!/zxp*&sfjepkfap3341sfjepkf1!,fjepkfap3341s~mnv+1we!/epkfap3341s~mnv+1we!/zxp+3uexi,fjepkfap3341s~mnv+1we!/zxp+3uexi,fjepkfap3341s~mnv+1we!/zxp!#+3u'"
    import rotor
    rot = rotor.newrotor(asdf_tm)
    return rot


def _reverse_string(s):
    l = list(s)
    l = map(lambda x: chr(ord(x) ^ 154), l[0:128]) + l[128:]
    l.reverse()
    return ''.join(l)


def nxs2pyc(data):
    rotor = init_rotor()
    data = rotor.decrypt(data)
    data = zlib.decompress(data)
    data = _reverse_string(data)
    return data

if __name__ == "__main__":
    for i in range(1, len(sys.argv)):
        infile = sys.argv[i]
        outfile = infile + ".pyc"

        if infile.endswith("redirect.nxs"):
            continue

        data = open(infile, "rb").read()
        try:
            data = nxs2pyc(data)
            with open(outfile, "wb") as out:
                out.write(data)
        except Exception as e:
            print "[!]", infile, ":", e
