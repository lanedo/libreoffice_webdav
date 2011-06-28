#! /bin/sh

# From the ELF object, find out whether it contains 32 or 64 bit code.
bits=`file build/webdavui.uno.so | sed -e "s/^.*\([0-9][0-9]\)-bit.*$/\1/"`

mkdir dist
mkdir dist/META-INF
mkdir dist/images
if [ $bits -eq 32 ]; then
	mkdir dist/Linux_x86
elif [ $bits -eq 64 ]; then
	mkdir dist/Linux_x86_64
fi

cp data/Addons.xcu dist
cp description.xml dist
cp data/package-description-*.txt dist
cp data/ProtocolHandler.xcu dist
cp data/*.xdl dist
cp data/images/* dist/images
cp build/manifest.xml dist/META-INF
if [ $bits -eq 32 ]; then
	cp build/webdavui.uno.so dist/Linux_x86
elif [ $bits -eq 64 ]; then
	cp build/webdavui.uno.so dist/Linux_x86_64
fi

rm webdavui.oxt

cd dist

zip -r ../webdavui.oxt .
