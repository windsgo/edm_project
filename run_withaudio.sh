#!/bin/bash

sudo /etc/init.d/ethercat restart;
# sudo pulseaudio --start --log-target=syslog && sudo build/App/app;
sudo build/App/app;
sudo /etc/init.d/ethercat stop;

# exec bash
