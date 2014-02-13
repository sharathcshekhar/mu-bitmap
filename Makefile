CC=gcc
CFLAGS=-c -g -std=c99 -D_GNU_SOURCE

default: all

all: build build_test test 

build: libmu_bitmap.a

libmu_bitmap.a: mu_bitmap.c mu_bitmap.h
	$(CC) $(CFLAGS) mu_bitmap.c -o mu_bitmap.o
	ar rcu libmu_bitmap.a mu_bitmap.o
	@echo "Successfully built library"

build_test: mu_bitmap_test.o libmu_bitmap.a
	$(CC) mu_bitmap_test.o -L. -lmu_bitmap -o unit_test
	@echo "Successfully built unit test"

mu_bitmap_test.o: mu_bitmap_test.c
	$(CC) $(CFLAGS) mu_bitmap_test.c -o mu_bitmap_test.o

test:
	@echo "Running unit tests"
	./unit_test

clean:
	rm -f *.o *.a unit_test
