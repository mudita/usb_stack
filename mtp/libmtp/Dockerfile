# Copyright  Onplick <info@onplick.com> - All Rights Reserved
# Unauthorized copying of this file, via any medium is strictly prohibited
# Proprietary and confidential

FROM  gcc

RUN apt-get update --yes && \
    apt-get install --yes cmake git
    
RUN git clone --depth=1 https://github.com/cgreen-devs/cgreen && \
    cd cgreen && \
    mkdir build && cd build && \
    cmake .. && \
    make && make install
    
RUN apt-get remove --yes cmake git
