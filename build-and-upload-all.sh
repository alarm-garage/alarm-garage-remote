#!/bin/bash

pio run --target upload && \
  pio device monitor
