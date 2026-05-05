#!/bin/bash

cd "$(dirname $0)"

if [ -d /usr/lib64/qt6/bin ]; then
	export PATH=$PATH:/usr/lib64/qt6/bin
fi

topdir="$(git rev-parse --show-toplevel)"

for i in *.ts ; do
	lupdate "${@}" $(git ls-files "${topdir}" | egrep -v '^tests/|../tests/' | egrep '\.cpp$|\.h$|\.ui$') -ts "${i}"
done
