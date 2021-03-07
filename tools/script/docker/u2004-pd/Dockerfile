# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#
# Build ubuntu-20.04 image with HPX and PCL 
#
FROM u2004-base

# Set metadata
LABEL maintainer="Prashant K. Jha <pjha.sci@gmail.com>"
LABEL version="0.1.0"
LABEL description="Ubuntu 20.04 with HPX and PCL installed"

# Set different working directory
WORKDIR /pd_builds/

# Copy the install script
COPY install.sh install.sh

# Run install script
RUN /bin/bash install.sh Release

# Copy the build logs
RUN cp build.log /pd_build.log

# Change to root
WORKDIR /

# Clean the build directories
RUN rm -rf /pd_builds



