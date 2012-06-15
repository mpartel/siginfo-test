#!/bin/sh -e
# Starts user-mode-linux with the test program available tarred in /dev/ubdb.
cd `dirname "$0"`

PATH_TO_UML=./linux
PATH_TO_INITRD=./initrd
PATH_TO_ROOTFS=./rootfs
MEM=128M
PARAMS=

if [ -f test-uml.settings ]; then
	. ./test-uml.settings
fi

rm -Rf tmp
mkdir tmp

mkdir -p input
cp -f test input/test

tar -C input -cf tmp/input.tar .

MAX_OUTPUT_SIZE=20M
dd if=/dev/zero of=tmp/output.tar bs=$MAX_OUTPUT_SIZE count=1

$PATH_TO_UML mem=$MEM \
	initrd=$PATH_TO_INITRD \
	ubdarc=$PATH_TO_ROOTFS \
	ubdbr=tmp/input.tar \
	ubdc=tmp/output.tar \
	$PARAMS
