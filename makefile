CC65 = cc65
CA65 = ca65
LD65 = ld65
NAME = NesPlatformerTemplate


$(NAME).nes: $(NAME).o reset.o asm4c.o nes.cfg
	$(LD65) -C nes.cfg -o $(NAME).nes --dbgfile vars.txt reset.o $(NAME).o asm4c.o nes.lib
# if linux uncomment the next line
	rm *.o
	rm $(NAME).s
# windows
#	del *.o
#	@echo $(NAME).nes created

reset.o: reset.s
	$(CA65) reset.s

asm4c.o: asm4c.s
	$(CA65) asm4c.s

$(NAME).o: $(NAME).s
	$(CA65) $(NAME).s

$(NAME).s: $(NAME).c
	$(CC65) -Oi $(NAME).c --add-source -g

clean:
# if linux uncomment the next line
	rm $(NAME).nes
# windows
#	del $(NAME).nes
