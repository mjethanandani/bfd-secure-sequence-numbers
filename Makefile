.PHONY: draft container push run all

leftover=$(shell docker ps -a -q -f status=exited)
leftover-image=$(shell docker images -a -q)
username=jhaas
image=$(username)/ietf-secure-sequence-numbers

all: container

container:
	-docker rm test
	docker build -t $(image) .; docker run -it --name test \
	--mount type=bind,src="$(PWD)",dst=/app \
	$(image)

debug:
	docker run -it $(image) bash

clean: packages-clean
	make -C draft clean
