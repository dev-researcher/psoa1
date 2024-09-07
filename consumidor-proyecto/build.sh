#!/bin/bash

rm -rf creador
rm -rf productor
rm -rf consumidor

gcc -o creador creador.c ../consumidor-gui/log_api.c ../consumidor-gui/buffer_api.c ../consumidor-gui/process_api.c -lpthread `pkg-config --cflags --libs gtk+-3.0`
gcc consumidor.c -o consumidor
gcc productor.c -o productor

