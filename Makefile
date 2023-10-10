CFLAGS = -g -Wall
PKGS=libavformat libavutil libavcodec
LDLIBS += `pkg-config --libs $(PKGS)`

ffremux: ffremux.o
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@


ffremux.o: ffremux.c
ffremux.o: CPPFLAGS += `pkg-config --cflags $(PKGS)`


ffremux_test: ffremux.o
	$(CC) -DTESTING -o $@ $^ $(LDLIBS)

%:
	@echo "Invalid target; I don't know how to make '$@'."

.PHONY: clean
clean:
	-rm -vf *.o ffremux ffremux_test


