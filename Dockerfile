FROM ubuntu:17.10

RUN apt update; apt install -y wget; \
  wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|apt-key add -;\
  echo 'deb http://apt.llvm.org/xenial/ llvm-toolchain-artful-5.0 main' >> /etc/apt/sources.list ; apt update;\
  apt install -y clang-5.0 lldb-5.0 lld-5.0 libc++-dev git cmake python-pip liblz4-dev; apt clean all


RUN cd ~; git clone https://github.com/reswitched/unicorn.git;\
  cd unicorn;\
  UNICORN_ARCHS="aarch64" ./make.sh;\
  ./make.sh install;\  
  cd ~; git clone https://github.com/reswitched/Mephisto.git; \
  cd Mephisto;\
  pip install -r requirements.txt;\
  make

EXPOSE 24689


ENTRYPOINT ["/root/Mephisto/ctu"]
CMD ["${*}"] 
