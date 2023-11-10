all: Fileread

.outputFolder:
	mkdir -p out
	
Fileread: .outputFolder
	clang -std=c++17 -lstdc++ -lm -Iinclude/ Fileread.cpp -Wall -Wextra -Werror -pthread -o out/ranges
	./out/ranges


