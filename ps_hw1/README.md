Useful commands:

```
gdb --args ./bin/prefix_scan -o temp.txt -n 12 -i tests/seq_63_test.txt -l 1 -a 0
valgrind --leak-check=yes ./bin/prefix_scan -o temp.txt -n 12 -i tests/seq_63_test.txt -l 1 -a 0
```
