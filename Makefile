CC = g++ 
NAME = ISA.cpp
RESULT = appdetector
LOGIN = xhaisv00
FILES = Makefile ISA.cpp

$(RESULT): $(NAME)
	$(CC) $(NAME) -o $(RESULT)
