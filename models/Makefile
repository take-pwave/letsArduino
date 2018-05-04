# match "module foobar() { // `make` me"
TARGETS=$(shell echo *.scad | sed -e 's/\.scad/.stl/g')

all: ${TARGETS}

# explicit wildcard expansion suppresses errors when no files are found
include $(wildcard *.deps)

%.stl: %.scad
	openscad -m make -o $@ -d $@.deps $<
