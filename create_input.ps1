#!/usr/bin/env bash

# download the specific dumps used to generate the reference solution small-ref.bz2
$DUMPVERSION=20220520

# download the latest data
# $DUMPVERSION=latest

$ProgressPreference = 'SilentlyContinue'

# 7zip gives Data Error while extracting large input file, thus using bzip2
mkdir bzip2
wget https://github.com/philr/bzip2-windows/releases/download/v1.0.8.0/bzip2-1.0.8.0-win-x64.zip -OutFile bzip2/bzip2-1.0.8.0-win-x64.zip
7z.exe x bzip2/bzip2-1.0.8.0-win-x64.zip -obzip2

wget https://dumps.wikimedia.org/huwikisource/$DUMPVERSION/huwikisource-$DUMPVERSION-pages-meta-current.xml.bz2 -OutFile huwikisource-$DUMPVERSION-pages-meta-current.xml.bz2
bzip2/bzip2.exe -d huwikisource-$DUMPVERSION-pages-meta-current.xml.bz2
mv huwikisource-$DUMPVERSION-pages-meta-current.xml data/small.data

wget https://dumps.wikimedia.org/huwiki/$DUMPVERSION/huwiki-$DUMPVERSION-pages-meta-current.xml.bz2 -OutFile huwiki-$DUMPVERSION-pages-meta-current.xml.bz2
bzip2/bzip2.exe -d huwiki-$DUMPVERSION-pages-meta-current.xml.bz2
mv huwiki-$DUMPVERSION-pages-meta-current.xml data/large.data

bzip2/bzip2.exe -d small-ref.bz2
mv small-ref ref/small.out
