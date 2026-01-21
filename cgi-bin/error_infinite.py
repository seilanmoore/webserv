#!/usr/bin/env python3
# Script with infinite loop - should trigger timeout
import time

while True:
    time.sleep(0.1)
