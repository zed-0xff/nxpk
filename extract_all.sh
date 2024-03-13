#!/usr/bin/env bash -e
# ^^^^^^^^^^^^ automatically use bash 5.x from homebrew on OSX

GAME_DIR=1.115

shopt -s globstar
cd $GAME_DIR

echo "[.] removing 'out' dir .."
rm -rf out

../nxpk.rb x **/*.npk

echo "[.] derotoring nxs scripts .."
docker build ../derotor -t derotor
docker run -ti -v .:/data derotor

echo "[.] decrypting redirect.nxs and neox.xml .."
../decrypt_xml.rb out/script.npk/redirect.nxs neox.xml

echo "[.] dexoring nxs scripts .."
../dexor_pyc.rb out/script.npk/redirect.nxs.decrypted out/script.npk/**/*.pyc

echo "[=] Done"
