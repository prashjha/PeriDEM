# dockerimages

Docker scripts can be found in [https://github.com/prashjha/dockerimages](https://github.com/prashjha/dockerimages). The details are as follows:

## building docker image ready for PeriDEM library
- [dockerimages/peridem-base](https://github.com/prashjha/dockerimages/tree/main/peridem-base) contains script that builds ubuntu image with all libraries installed. It takes as an argument UBUNTU_NAME during docker build. See file [Dockerfile-peridem-base-ubuntu-name-arg](https://github.com/prashjha/dockerimages/blob/main/peridem-base/Dockerfile-peridem-base-ubuntu-name-arg).

## building and testing PeriDEM using docker
- [dockerimages/other/peridem-test-use-peridem-base-image-ubuntu-name-arg](https://github.com/prashjha/dockerimages/tree/main/other/peridem-test-use-peridem-base-image-ubuntu-name-arg) contains script that will use one of the peridem-base images (e.g., peridem-base-focal) and build and test PeriDEM library. See file [Dockerfile-peridem-test-use-peridem-base-image-ubuntu-name-arg](https://github.com/prashjha/dockerimages/blob/main/other/peridem-test-use-peridem-base-image-ubuntu-name-arg/Dockerfile-peridem-test-use-peridem-base-image-ubuntu-name-arg).


- [dockerimages/other/peridem-test-fresh-build-peridem-base-ubuntu-name-arg] contains script that will take fresh ubuntu and install essential libraries and build and test PeriDEM. That is we combine the steps in [dockerimages/peridem-base](https://github.com/prashjha/dockerimages/tree/main/peridem-base) and [dockerimages/other/peridem-test-use-peridem-base-image-ubuntu-name-arg](https://github.com/prashjha/dockerimages/tree/main/other/peridem-test-use-peridem-base-image-ubuntu-name-arg) into one file. See file [Dockerfile-peridem-test-fresh-build-peridem-base-ubuntu-name-arg](https://github.com/prashjha/dockerimages/blob/main/other/peridem-test-fresh-build-peridem-base-ubuntu-name-arg/Dockerfile-peridem-test-fresh-build-peridem-base-ubuntu-name-arg).

## clion remote development

- [dockerimages/other/peridem-clion-remote-use-peridem-base-image-ubuntu-name-arg](https://github.com/prashjha/dockerimages/tree/main/other/peridem-clion-remote-use-peridem-base-image-ubuntu-name-arg) contains script that builds ubuntu image for clion remote development. that will use one of the peridem-base images (e.g., peridem-base-focal). See file [Dockerfile-peridem-clion-use-peridem-base-image-ubuntu-name-arg](https://github.com/prashjha/dockerimages/blob/main/other/peridem-clion-remote-use-peridem-base-image-ubuntu-name-arg/Dockerfile-peridem-clion-use-peridem-base-image-ubuntu-name-arg).
