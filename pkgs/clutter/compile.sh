#!/bin/sh

export CFLAGS="-DK_SAGEM_HOMESCREEN=1" 
export LDFLAGS="-lGLES_CM" 

./configure --with-flavour=eglx --prefix=/usr/

make
make install
