CC = gcc
# gnu99 + _GNU_SOURCE: strdup, fileno in generated lex.yy.c on MinGW
CFLAGS = -std=gnu99 -Wall -D_GNU_SOURCE
FLEX = flex
BISON = bison

TARGET = ekker
INPUT = sample.txt

GEN = lex.yy.c parser.tab.c
HDR = parser.tab.h
SRC = main.c symtab.c

.PHONY: all run clean

all: $(TARGET)

$(HDR) parser.tab.c: parser.y
	$(BISON) -d -o parser.tab.c parser.y

# GnuWin32 flex 2.5.x: `-o lex.yy.c` (with space) is parsed wrong → "can't open lex.yy.c".
# Default output is lex.yy.c, or use `-olex.yy.c` with no space.
lex.yy.c: scanner.l $(HDR)
	$(FLEX) scanner.l

$(TARGET): $(GEN) $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(GEN) -o $(TARGET)

run: all
	./$(TARGET) $(INPUT)

# Windows: mingw32-make has no `rm` on PATH — use cmd. Unix: use rm.
clean:
ifdef ComSpec
	-$(ComSpec) /C "del /F /Q $(TARGET).exe $(TARGET) lex.yy.c parser.tab.c parser.tab.h *.o 2>nul"
else
	rm -f $(TARGET) $(TARGET).exe $(GEN) $(HDR) *.o
endif
