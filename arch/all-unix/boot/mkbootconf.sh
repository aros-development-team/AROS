#!/bin/sh

sed "s/@arch@/$2/" ${1-.}/AROSBootstrap.conf
