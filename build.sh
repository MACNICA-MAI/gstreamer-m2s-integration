#!/usr/bin/env bash
_H=$(cd $(dirname ${BASH_SOURCE:-$0}); pwd)

g++ -Wall -shared -fPIC -o ${_H}/gstm2svideosrc.so \
    ${_H}/src/gstm2svideosrc.cpp -I${_H}/../library/include \
    -L${_H}/../library -lrt -lm2s `pkg-config --cflags --libs gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0` -std=gnu++11 &&
g++ -Wall -shared -fPIC -o ${_H}/gstm2svideosink.so \
    ${_H}/src/gstm2svideosink.cpp ${_H}/../common/tr_offset.c -I${_H}/../common -I${_H}/../library/include \
    -L${_H}/../library -lrt -lm2s `pkg-config --cflags --libs gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0` -std=gnu++11 &&
g++ -Wall -shared -fPIC -o ${_H}/gstm2saudiosink.so \
    ${_H}/src/gstm2saudiosink.cpp ${_H}/../common/tr_offset.c -I${_H}/../common -I${_H}/../library/include \
    -L${_H}/../library -lrt -lm2s `pkg-config --cflags --libs gstreamer-1.0 gstreamer-base-1.0 gstreamer-audio-1.0` -std=gnu++11 &&
g++ -Wall -shared -fPIC -o ${_H}/gstm2saudiosrc.so \
    ${_H}/src/gstm2saudiosrc.cpp -I${_H}/../library/include \
    -L${_H}/../library -lrt -lm2s `pkg-config --cflags --libs gstreamer-1.0 gstreamer-base-1.0 gstreamer-audio-1.0` -std=gnu++11
