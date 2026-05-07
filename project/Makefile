CC      = gcc
CFLAGS  = -Wall -Wextra -g
TARGET  = myParser

all: $(TARGET)

$(TARGET): mysqlq.tab.c mysqlq.tab.h lex.yy.c symtable.c
	$(CC) $(CFLAGS) -o $(TARGET) mysqlq.tab.c lex.yy.c symtable.c

mysqlq.tab.c mysqlq.tab.h: mysqlq.y
	bison -d mysqlq.y

lex.yy.c: mysqlq.l mysqlq.tab.h
	flex mysqlq.l

clean:
	rm -f mysqlq.tab.c mysqlq.tab.h lex.yy.c $(TARGET) *.o

test: $(TARGET)
	@echo "=== Test 1: Valid CREATE TABLE ==="
	./$(TARGET) tests/test1_create_ok.sql
	@echo ""
	@echo "=== Test 2: Valid SELECT ==="
	./$(TARGET) tests/test2_select_ok.sql
	@echo ""
	@echo "=== Test 3: Syntax error ==="
	-./$(TARGET) tests/test3_syntax_error.sql
	@echo ""
	@echo "=== Test 4: Semantic checks (Q2) ==="
	./$(TARGET) tests/test4_semantic_ok.sql
	@echo ""
	@echo "=== Test 5: Semantic error — undefined table ==="
	-./$(TARGET) tests/test5_semantic_error.sql
	@echo ""
	@echo "=== Test 6: JOIN (Q3) ==="
	./$(TARGET) tests/test6_join_ok.sql
	@echo ""
	@echo "=== Test 7: Aliases (Q3) ==="
	./$(TARGET) tests/test7_alias_ok.sql
	@echo ""
	@echo "=== Test 8: Semantic error — unknown column ==="
	-./$(TARGET) tests/test8_column_error.sql
	@echo ""
	@echo "=== Test 9: Type mismatch in WHERE ==="
	-./$(TARGET) tests/test9_type_error.sql
	@echo ""
	@echo "=== Test 10: Complex query with comments ==="
	./$(TARGET) tests/test10_comments.sql

.PHONY: all clean test
