PROGRAM=ScratchFirmata

all: $(PROGRAM)

OBJS = Serial.o ScratchFirmata.o

PKG_CONFIG=
PKG_CONFIG_CFLAGS=`wx-config --cflags`
PKG_CONFIG_LIBS=`wx-config --libs`

EXTRA_CFLAGS=$(PKG_CONFIG_CFLAGS) -DLINUX
CFLAGS= -O2 -g -Wall

LDFLAGS= -Wl,-z,defs -Wl,--as-needed -Wl,--no-undefined
EXTRA_LDFLAGS=
LIBS=$(PKG_CONFIG_LIBS)

$(PROGRAM): $(OBJS)
	g++ $(LDFLAGS) $(EXTRA_LDFLAGS) $+ -o $@ $(LIBS)

%.o: %.cpp
	g++ -o $@ -c $+ $(CFLAGS) $(EXTRA_CFLAGS)

%.o: %.c
	gcc -o $@ -c $+ $(CFLAGS) $(EXTRA_CFLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(PROGRAM)
	rm -f *.o *.a *~

