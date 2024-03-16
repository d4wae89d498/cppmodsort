NAME=cppmodsort

all: $(NAME) test

$(NAME): cppmodsort.cpp
	clang++ $<  -o $@ -I /usr/local/opt/llvm/include /usr/local/opt/llvm/lib/libclang.dylib

test:
	make -C examples/raw
	make -C examples/partitions

clean:
	make clean -C examples/raw
	make clean -C examples/partitions
	rm $(NAME)

re: clean all
