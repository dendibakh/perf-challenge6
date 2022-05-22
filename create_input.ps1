#!/usr/bin/env bash

wget http://dumps.wikimedia.org/huwikisource/latest/huwikisource-latest-pages-meta-current.xml.bz2 -OutFile huwikisource-latest-pages-meta-current.xml.bz2
7z.exe x huwikisource-latest-pages-meta-current.xml.bz2
mv huwikisource-latest-pages-meta-current.xml data/small.data

wget http://dumps.wikimedia.org/huwiki/latest/huwiki-latest-pages-meta-current.xml.bz2 -OutFile huwiki-latest-pages-meta-current.xml.bz
7z.exe x huwiki-latest-pages-meta-current.xml.bz2
mv huwiki-latest-pages-meta-current.xml data/large.data

7z.exe x small-ref.bz2
mv small-ref ref/small.out