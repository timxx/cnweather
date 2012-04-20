#!/bin/bash
set -x
aclocal
autoconf
libtoolize --copy --force --automake
intltoolize --force
autoreconf --install --force
