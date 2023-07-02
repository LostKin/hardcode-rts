find . -regex '.*\.\(cpp\|h\)' -not -path '*/\.*' -exec clang-format-14 -style=file:.clang-format -i {} \;
