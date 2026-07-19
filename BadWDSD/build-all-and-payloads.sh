#!/bin/bash

./build-payloads.sh || exit 1
./build-all.sh || exit 1