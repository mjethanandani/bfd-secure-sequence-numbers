/*
------------------------------------------------------------------------------
rand.c: By Bob Jenkins.  My random number generator, ISAAC.  Public
Domain MODIFIED: 960327: Creation (addition of randinit, really)
970719: use context, not global variables, for internal state 980324:
make a portable version 010626: Note this is public domain 100725:
Mask on use of >32 bits, not on assignment: from Paul Eggert
------------------------------------------------------------------------------
*/

/*
 *	Modified
 *	* be a stand-alone C file
 *	* use modern types,
 *	* change the formatting to add more whitespace.
 *	* remove "& 0xffffffff" everywhere, as types are now explicitly 32-bits
 *	* randinit() gets passed a seed pointer, not a flag
 *	* main() allows for passing a seed on the command line.
 *	* move random variables to arrays.  It's less code, and allows for more
 *	  compiler optimizations
 *	* rename variables to be a little clearer.
 */

#define RANDSIZL   (8)
#define RANDSIZ    (1 << RANDSIZL)

#include <stdint.h>
#include <string.h>

/* context of random number generator */
typedef struct randctx {
	uint32_t remaining;			// unused nunbers remaining in this page
	uint32_t randa;
	uint32_t randb;
	uint32_t num_pages;			// number of pages we have generated.

	uint32_t page[RANDSIZ];			// current page of 256 random numbers to output
	uint32_t pool[RANDSIZ];			// pool of secret entropy
} randctx;

#define ind(x)  (start[(x >> 2) & (RANDSIZ - 1)])
#define rngstep(mix,a,b,m,m2,r) \
{ \
	uint32_t y, x = *m;		        \
	a = ((a^(mix)) + *(m2++));              \
	*(m++) = y = (ind(x) + a + b);	        \
	*(r++) = b = (ind(y >> RANDSIZL) + x) ;	\
}

static void isaac(randctx *ctx)
{
	uint32_t a,b;
	uint32_t *start, *end;
	uint32_t *m, *m2, *r;

	start = ctx->pool;
	r = ctx->page;

	a = ctx->randa;
	b = ctx->randb + (++ctx->num_pages);

	for (m = start, end = m2 = m + (RANDSIZ / 2); m < end; ) {
		rngstep( a << 13, a, b, m, m2, r);
		rngstep( a >> 6 , a, b, m, m2, r);
		rngstep( a << 2 , a, b, m, m2, r);
		rngstep( a >> 16, a, b, m, m2, r);
	}

	for (m2 = start; m2 < end; ) {
		rngstep( a << 13, a, b, m, m2, r);
		rngstep( a >> 6 , a, b, m, m2, r);
		rngstep( a << 2 , a, b, m, m2, r);
		rngstep( a >> 16, a, b, m, m2, r);
	}

	ctx->randb = b;
	ctx->randa = a;
}


#define mix(a) \
{ \
	a[0] ^= a[1] << 11;  a[3] += a[0]; a[1] += a[2];	\
	a[1] ^= a[2] >> 2;   a[4] += a[1]; a[2] += a[3];	\
	a[2] ^= a[3] << 8;   a[5] += a[2]; a[3] += a[4];	\
	a[3] ^= a[4] >> 16;  a[6] += a[3]; a[4] += a[5];	\
	a[4] ^= a[5] << 10;  a[7] += a[4]; a[5] += a[6];	\
	a[5] ^= a[6] >> 4;   a[0] += a[5]; a[6] += a[7];	\
	a[6] ^= a[7] << 8;   a[1] += a[6]; a[7] += a[0];	\
	a[7] ^= a[0] >> 9;   a[2] += a[7]; a[0] += a[1];	\
}

/*
 *	Initialize the random context with a seed.  If there's no
 *	seed, just use all zeroes.
 */
void isaac_randinit(randctx *ctx, void const *seed, int seedlen)
{
	int i, j;
	uint32_t a[8];
	uint32_t *m,*r;

	memset(ctx, 0, sizeof(*ctx));

	if (seed && (seedlen > 0)) {
		if (seedlen > sizeof(ctx->page)) seedlen = sizeof(ctx->page);

		memcpy(ctx->page, seed, seedlen);
	}

	m = ctx->pool;
	r = ctx->page;

	for (j = 0; j < 8; j++) a[j] = 0x9e3779b9;  /* the golden ratio */

	for (i = 0; i < 4; ++i) {          /* scramble it */
		mix(a);
	}

	/* initialize using the contents of pages[] as the seed */
	for (i = 0; i < RANDSIZ; i += 8) {
		for (j = 0; j < 8; j++) a[j] += r[i + j];

		mix(a);

		memcpy(&m[i], a, sizeof(a));
	}

	/* do a second pass to make all of the seed affect all of the entropy pool */
	for (i = 0; i < RANDSIZ; i += 8) {
		for (j = 0; j < 8; j++) a[j] += m[i + j];

		mix(a);

		memcpy(&m[i], a, sizeof(a));
	}

	isaac(ctx);            /* fill in the first set of results */
	ctx->remaining = RANDSIZ;  /* prepare to use the first set of results */
}

uint32_t isaac_rand(randctx *ctx)
{
	if (ctx->remaining > 0) {
		 ctx->remaining--;
		 return ctx->page[ctx->remaining];
	}

	isaac(ctx);

	ctx->remaining = RANDSIZ - 1;
	return ctx->page[RANDSIZ - 1];
}

#ifdef NEVER
#include <stdio.h>

int main(int argc, char const *argv[])
{
	uint32_t i,j;
	randctx ctx;
	
	if (argc == 1) {
		isaac_randinit(&ctx, NULL, 0);
	} else {
		isaac_randinit(&ctx, argv[1], strlen(argv[1]));
	}

	for (i = 0; i < 2; ++i) {
		isaac(&ctx);

		for (j = 0; j < 256; ++j) {
			printf("%.8x", ctx.page[j]);

			if ((j&7) == 7) printf("\n");
		}
	}
}
#endif

