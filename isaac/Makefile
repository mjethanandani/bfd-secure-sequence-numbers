all: diff sequence

clean:
	@rm -f randport randport.txt modern.txt sequence *.o *~

diff: randdvec.txt randport.txt modern.txt
	@diff randdvec.txt randport.txt || echo "randport.txt is not correct"
	@diff randdvec.txt modern.txt || echo "modern.txt is not correct"

#
#  Wildcard rule to build output files
#
%.txt: %
	@./$^ > $@

#
#  The original file
#
randport: randport.c standard.h rand.h
	@$(CC) -DNEVER $< -o $@

#
#  Various cleanups
#
modern: modern.c
	@$(CC) -g -DNEVER $< -o $@

#
#  Tracking BFD sequence numbers
#
sequence: sequence.c
	@$(CC) -DNEVER $< -o $@

aes_example: aes_example.c
	@$(CC) -DNEVER $< -o $@ -l crypto
