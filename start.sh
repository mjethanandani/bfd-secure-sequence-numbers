#!/bin/sh

cp /app/.gitconfig /root/.gitconfig
cd /app/draft
make clean
make
