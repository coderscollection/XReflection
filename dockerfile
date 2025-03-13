#dockerfile
#(c) John Nessworthy 2024 all rights reserved.

#build an ubuntu image

FROM ubuntu:22.04
RUN apt-get update -y
RUN apt-get upgrade -y

#compilation tools
RUN apt-get install -y build-essential cmake

#common stuff
RUN apt-get install -y apt-transport-https software-properties-common ca-certificates curl openssl wget

#source code,editor,json helper and wcat used in test scripts
RUN apt-get install -y git vim jq node-ws

#websocat used for some tests
RUN wget -qO /usr/local/bin/websocat https://github.com/vi/websocat/releases/latest/download/websocat.x86_64-unknown-linux-musl
RUN chmod a+x /usr/local/bin/websocat

#useful libraries for our project
RUN apt-get install -y libcurl4-openssl-dev 

#I'm going to use a header-only library I wrote a few years ago to handle JSON
#purely because it's likely to make the code cleaner and demonstrate some external project integration.
#I realize this makes the project more complex - but it's also more similar to what a real commercial
#project looks like.  My header relies on boost - so - in it goes.


#I have elected to just include the .tar file in the repo rather than fetching it as I originally intended
#just because it runs quicker with less reliance on external internet vagaries this way and was
#quicker to test with.

RUN cd /home && wget http://downloads.sourceforge.net/project/boost/boost/1.69.0/boost_1_69_0.tar.gz \
  && tar xfz boost_1_69_0.tar.gz \
  && cd boost_1_69_0 \
  && ./bootstrap.sh --prefix=/usr/local --with-libraries=program_options \
  && ./b2 install \
  && cd /home \
  && rm -rf boost_1_69_0

#create build user
RUN useradd -ms /bin/bash build


