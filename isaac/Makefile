all: diff

clean:
	@rm -f randport randport.txt *~

diff: randdvec.txt randport.txt modern.txt
	@diff randdvec.txt randport.txt
	@diff randdvec.txt modern.txt

randport.txt: randport
	@./randport > $@

randport: randport.c standard.h rand.h
	@$(CC) -DNEVER $< -o $@

modern.txt: modern
	@./modern > $@

modern: modern.c standard.h rand.h
	@$(CC) -DNEVER $< -o $@
