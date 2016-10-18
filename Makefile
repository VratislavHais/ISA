CC = g++ 
NAME = ISA.cpp
RESULT = isa
LOGIN = xhaisv00
FILES = Makefile ISA.cpp

$(RESULT): $(NAME)
	$(CC) $(NAME) -o $(RESULT)
