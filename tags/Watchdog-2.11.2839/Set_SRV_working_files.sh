#!/bin/sh

mkdir ~watchdog/Gif
mkdir ~watchdog/Son
mkdir ~watchdog/Dls
mkdir ~watchdog/RRA
mkdir ~watchdog/WEB
mkdir ~watchdog/WEB/img

cp -rv Gif/* ~watchdog/Gif
cp -rv Son/* ~watchdog/Son

echo "Directory created and Files copied for SRV"
