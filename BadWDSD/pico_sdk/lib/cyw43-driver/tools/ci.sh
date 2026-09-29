#!/bin/bash

if which nproc > /dev/null; then
    MAKEOPTS="-rRj$(nproc)"
else
    MAKEOPTS="-rRj$(sysctl -n hw.ncpu)"
fi

########################################################################################
# code formatting

function ci_code_formatting_setup {
    sudo apt-get install uncrustify
    pip3 install black
    uncrustify --version
    black --version
}

function ci_code_formatting_run {
    tools/codeformat.py -v
}

########################################################################################
# code spelling

function ci_code_spell_setup {
    pip install codespell
}

function ci_code_spell_run {
    codespell firmware/ src/
}

########################################################################################
# tests

function ci_tests_setup {
    sudo dpkg --add-architecture i386
    sudo apt-get update
    sudo apt-get install gcc-multilib
}

function ci_tests_build {
    for test in `ls -d tests/*/`; do
        make $MAKEOPTS -C $test
    done
}

function ci_tests_run {
    for test in `ls -d tests/*/`; do
        make $MAKEOPTS -C $test test
    done
}

########################################################################################
# tests with clang

function ci_tests_clang_setup {
    sudo apt-get install clang gcc-multilib
}

function ci_tests_clang_build {
    for test in `ls -d tests/*/`; do
        make $MAKEOPTS CC=clang -C $test
    done
}

function ci_tests_clang_run {
    for test in `ls -d tests/*/`; do
        make $MAKEOPTS CC=clang -C $test test
    done
}
