FROM gitpod/workspace-full

USER root

RUN apt install -y \
	libc6-dev 