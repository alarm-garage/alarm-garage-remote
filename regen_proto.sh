#!/bin/bash

cd src
~/sw/nanopb/protoc --nanopb_out=proto alarm.proto
sed -i 's/#include <pb.h>/#include <proto\/pb.h>/g' proto/alarm.pb.h
cd ..
