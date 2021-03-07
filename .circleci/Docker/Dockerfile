# ----------------------------------
# Copyright (c) 2021 Prashant K. Jha
# ----------------------------------
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#
# Build ubuntu-18.04 image with HPX and PCL 
#
FROM prashjha/u1804-pd:latest

# Set metadata
LABEL maintainer="Prashant K. Jha <pjha.sci@gmail.com>"
LABEL version="0.1.0"
LABEL description="PeriDEM docker image"

WORKDIR /

RUN git clone  https://github.com/prashjha/PeriDEM.git

RUN cd /PeriDEM && \
		mkdir build && \
		cd build && \
		cmake -DHPX_IGNORE_COMPILER_COMPATIBILITY=ON \
					-DCMAKE_BUILD_TYPE=Release \
					.. && \
		make -j 2 && \
		make install

