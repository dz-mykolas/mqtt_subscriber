BIN:=mqtt_subscriber
SRC:=$(wildcard *.c)
OBJ:=$(SRC:.c=.o)
CPPFLAGS:=
LDFLAGS:=-lmosquitto -luci -lcurl -largp

.PHONY: all clean $(BIN)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
clean:
	$(RM) $(OBJ) $(BIN)