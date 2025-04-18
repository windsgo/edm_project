#!/bin/bash
sudo apt-get install libsocketcan-dev ethtool 
sudo apt-get install autoconf pkgconf libtool libsysfs-dev # igh bootstrap
sudo apt-get install freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev # qt5
sudo apt install stress rt-tests # rt test
sudo apt install libboost-all-dev
sudo apt install dbus-x11
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-13 g++-13