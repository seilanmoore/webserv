#!/bin/bash

# Stress test for webserv
REQUESTS=${1:-100}
URL=${2:-"http://localhost:8080/"}

echo "Running stress test: $REQUESTS requests to $URL"

SUCCESS=0
FAIL=0

for i in $(seq 1 $REQUESTS); do
    CODE=$(curl -s -o /dev/null -w "%{http_code}" "$URL" 2>/dev/null)
    if [ "$CODE" = "200" ]; then
        SUCCESS=$((SUCCESS+1))
    else
        FAIL=$((FAIL+1))
    fi
done

echo "==============================="
echo "Total: $REQUESTS"
echo "Success (200): $SUCCESS"
echo "Failed: $FAIL"
RATE=$((SUCCESS * 100 / REQUESTS))
echo "Success rate: ${RATE}%"
echo "==============================="

if [ $SUCCESS -ge $((REQUESTS * 9 / 10)) ]; then
    echo "✅ PASS (>90% success)"
    exit 0
else
    echo "❌ FAIL (<90% success)"
    exit 1
fi
