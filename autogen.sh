autoreconf -if || exit $?
test -n $NOCONFIGURE && ./configure $@
