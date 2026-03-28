CC = gcc
FLEX = flex
BISON = bison

# MinGW: strdup/fileno; quiet warnings from generated lex.yy.c only
CFLAGS = -std=gnu99 -D_GNU_SOURCE -Wno-unused-label -Wno-unused-function

TARGET = ekker

all:
	$(BISON) -d -o parser.tab.c parser.y
	$(FLEX) scanner.l
	$(CC) $(CFLAGS) lex.yy.c parser.tab.c symtab.c main.c -o $(TARGET)

run:
	./$(TARGET) sample.txt

clean:
	del lex.yy.c parser.tab.c parser.tab.h ekker.exe
