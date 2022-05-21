#!/usr/bin/env bash

wget http://dumps.wikimedia.org/huwikisource/latest/huwikisource-latest-pages-meta-current.xml.bz2
bunzip2 huwikisource-latest-pages-meta-current.xml.bz2
mv huwikisource-latest-pages-meta-current.xml data/small.data

wget http://dumps.wikimedia.org/huwiki/latest/huwiki-latest-pages-meta-current.xml.bz2
bunzip2 huwiki-latest-pages-meta-current.xml.bz2
mv huwiki-latest-pages-meta-current.xml data/large.data

tar -xf small.tar.xz -C ref