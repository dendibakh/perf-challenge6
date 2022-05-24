#!/usr/bin/env bash

$ProgressPreference = 'SilentlyContinue'

# 7zip gives Data Error while extracting large input file, thus using bzip2
mkdir bzip2
wget https://github.com/philr/bzip2-windows/releases/download/v1.0.8.0/bzip2-1.0.8.0-win-x64.zip -OutFile bzip2/bzip2-1.0.8.0-win-x64.zip
7z.exe x bzip2/bzip2-1.0.8.0-win-x64.zip -obzip2

wget http://dumps.wikimedia.org/huwikisource/latest/huwikisource-latest-pages-meta-current.xml.bz2 -OutFile huwikisource-latest-pages-meta-current.xml.bz2
bzip2/bzip2.exe -d huwikisource-latest-pages-meta-current.xml.bz2
mv huwikisource-latest-pages-meta-current.xml data/small.data

wget http://dumps.wikimedia.org/huwiki/latest/huwiki-latest-pages-meta-current.xml.bz2 -OutFile huwiki-latest-pages-meta-current.xml.bz2
bzip2/bzip2.exe -d huwiki-latest-pages-meta-current.xml.bz2
mv huwiki-latest-pages-meta-current.xml data/large.data

bzip2/bzip2.exe -d small-ref.bz2
mv small-ref ref/small.out
