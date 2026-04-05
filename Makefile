CC = gcc
FLEX = flex
BISON = bison

CFLAGS = -std=gnu99 -D_GNU_SOURCE -Wno-unused-label -Wno-unused-function

TARGET = ekker

all:
	$(BISON) -d -o parser.tab.c parser.y
	$(FLEX) scanner.l
	$(CC) $(CFLAGS) lex.yy.c parser.tab.c ast.c symtab.c ir.c optimize.c codegen.c main.c -o $(TARGET)

run:
	./$(TARGET) sample.txt

clean:
	del lex.yy.c parser.tab.c parser.tab.h ekker.exe

