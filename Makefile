
format:
	find . -type f -name "*.c" -o -name "*.h" | xargs clang-format -style=file -i
