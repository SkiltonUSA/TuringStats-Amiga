.PHONY: build artwork
build:
	sh amiga/turing/build.sh "$(CURDIR)/build"
artwork:
	python3 amiga/turing/make-art.py
