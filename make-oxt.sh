#! /bin/sh

mkdir dist
mkdir dist/META-INF
mkdir dist/images
mkdir dist/Linux_x86

cp data/Addons.xcu dist
cp description.xml dist
cp data/package-description.txt dist
cp data/ProtocolHandler.xcu dist
cp data/images/* dist/images
cp build/manifest.xml dist/META-INF
cp build/webdavui.uno.so dist/Linux_x86

rm webdavui.oxt

cd dist

zip -r ../webdavui.oxt .
