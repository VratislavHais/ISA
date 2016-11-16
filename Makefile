CC = g++ 
NAME = appdetector.cpp
RESULT = appdetector
LOGIN = xhaisv00
FILES = Makefile appdetector.cpp

$(RESULT): $(NAME)
	$(CC) $(NAME) -o $(RESULT)
