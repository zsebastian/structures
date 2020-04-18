src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = -lm

test: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) test
