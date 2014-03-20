#!/bin/bash 

tar cjvf /mnt/hgfs/e_drive/horoscope.tar.bz2 . --exclude .git --exclude *.html --exclude *.jpeg --exclude *.doc* --exclude .svn --exclude bins
