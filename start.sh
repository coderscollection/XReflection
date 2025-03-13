#!/bin/sh

#make our container. Safer to use sudo. 
sudo docker build --tag test:latest .

#run our container, mount our source folder in it amd log on as the build user
sudo docker run  -v ./project:/home/build  -u build -it test bash


