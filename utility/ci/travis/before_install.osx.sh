#!/bin/bash -ue

HOMEBREW_FORMULAS="sqlite unixodbc sqliteodbc cmake"

brew update
brew upgrade

IFS=' '
for I in $HOMEBREW_FORMULAS; do
    brew install $I
done
