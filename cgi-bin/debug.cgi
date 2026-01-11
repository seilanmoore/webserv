#!/bin/sh
echo "Content-Type: text/plain"
echo ""
echo "Environment:"
env | sort
echo ""
echo "stdin:"
cat
