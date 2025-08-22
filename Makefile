
CREATOR =  gcc -Wall -ansi -pedantic 
TARGET = assembler 
OBJS = assembler.o utils.o data_structures.o pre_assembler.o first_pass.o second_pass.o


$(TARGET): $(OBJS)
	$(CREATOR) -o $@ $(OBJS)

assembler.o: assembler.c data_structures.h pre_assembler.h first_pass.h second_pass.h
	$(CREATOR) -c assembler.c -o $@

utils.o: utils.c utils.h
	$(CREATOR) -c utils.c -o $@

data_structures.o: data_structures.c data_structures.h
	$(CREATOR) -c data_structures.c -o $@

pre_assembler.o: pre_assembler.c pre_assembler.h data_structures.h utils.h
	$(CREATOR) -c pre_assembler.c -o $@

first_pass.o: first_pass.c first_pass.h data_structures.h utils.h
	$(CREATOR) -c first_pass.c -o $@

second_pass.o: second_pass.c second_pass.h data_structures.h utils.h
	$(CREATOR) -c second_pass.c -o $@

clean:
	rm -f $(TARGET) $(OBJS) *.am *.ob *.ent *.ext
