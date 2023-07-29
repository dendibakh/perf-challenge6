#!/usr/bin/env bash

# Download the specific dumps used to generate the reference solution small-ref.bz2
DUMPVERSION=20220520

# Download the latest data instead.  Note that 1st of the month full are significantly different
# from 20th of the month partial dumps.  20220520 is a partial dump
# https://meta.wikimedia.org/wiki/Data_dumps/Dump_frequency
# DUMPVERSION=latest

wget https://dumps.wikimedia.your.org/huwikisource/$DUMPVERSION/huwikisource-$DUMPVERSION-pages-meta-current.xml.bz2
bunzip2 huwikisource-$DUMPVERSION-pages-meta-current.xml.bz2
mv huwikisource-$DUMPVERSION-pages-meta-current.xml data/small.data

wget https://dumps.wikimedia.your.org/huwiki/$DUMPVERSION/huwiki-$DUMPVERSION-pages-meta-current.xml.bz2
bunzip2 huwiki-$DUMPVERSION-pages-meta-current.xml.bz2
mv huwiki-$DUMPVERSION-pages-meta-current.xml data/large.data

bunzip2 small-ref.bz2
mv small-ref ref/small.out