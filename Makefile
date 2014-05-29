CFLAGS=-Wall -g
common_objs=mmap_posix.o util.o
print_db_ns_objs=print_db_ns.o
dump_objs=dump.o datafile_manager.o

all:print_db_ns dump

print_db_ns:$(common_objs) $(print_db_ns_objs)
	g++ $(CFLAGS) -o print_db_ns $(common_objs) $(print_db_ns_objs)

dump:$(common_objs) $(dump_objs)
	g++ $(CFLAGS) -o dump $(common_objs) $(dump_objs)

$(common_objs): %.o: %.cpp
	g++ -c $(CFLAGS) $< -o $@

$(print_db_ns_objs): %.o: %.cpp
	g++ -c $(CFLAGS) $< -o $@
	
$(dump_objs): %.o: %.cpp
	g++ -c $(CFLAGS) $< -o $@

clean:
	rm -f print_db_ns dump *.o


