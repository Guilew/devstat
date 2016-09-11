FLAGS+= -Os -fPIC
FLAGS+= -Wall -Werror

DEFS= _GNU_SOURCE

CFLAGS=$(FLAGS) $(addprefix -D,$(DEFS))

devstat: devstat.o
	$(CC) -o $@ devstat.o $(LD_FLAGS) $(FLAGS)	

clean:
	rm -f *.o devstat  

