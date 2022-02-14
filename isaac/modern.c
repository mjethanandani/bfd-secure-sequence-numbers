/*
------------------------------------------------------------------------------
rand.c: By Bob Jenkins.  My random number generator, ISAAC.  Public Domain
MODIFIED:
  960327: Creation (addition of randinit, really)
  970719: use context, not global variables, for internal state
  980324: make a portable version
  010626: Note this is public domain
  100725: Mask on use of >32 bits, not on assignment: from Paul Eggert
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
 */

#define RANDSIZL   (8)
#define RANDSIZ    (1 << RANDSIZL)

#include <stdint.h>
#include <string.h>

/* context of random number generator */
typedef struct randctx {
	uint32_t randcnt;
	uint32_t randa;
	uint32_t randb;
	uint32_t randc;

	uint32_t randrsl[RANDSIZ];
	uint32_t randmem[RANDSIZ];
} randctx;

#define ind(mm,x)  ((mm)[(x>>2)&(RANDSIZ-1)])
#define rngstep(mix,a,b,mm,m,m2,r,x) \
{ \
	x = *m;		                                \
	a = ((a^(mix)) + *(m2++));                      \
	*(m++) = y = (ind(mm, x) + a + b);	        \
	*(r++) = b = (ind(mm, y >> RANDSIZL) + x) ;	\
}

static void isaac(randctx *ctx)
{
	uint32_t a,b,x,y,*m,*mm,*m2,*r,*mend;

	mm = ctx->randmem;
	r = ctx->randrsl;

	a = ctx->randa;
	b = ctx->randb + (++ctx->randc);

	for (m = mm, mend = m2 = m + (RANDSIZ/2); m < mend; ) {
		rngstep( a << 13, a, b, mm, m, m2, r, x);
		rngstep( a >> 6 , a, b, mm, m, m2, r, x);
		rngstep( a << 2 , a, b, mm, m, m2, r, x);
		rngstep( a >> 16, a, b, mm, m, m2, r, x);
	}

	for (m2 = mm; m2 < mend; ) {
		rngstep( a << 13, a, b, mm, m, m2, r, x);
		rngstep( a >> 6 , a, b, mm, m, m2, r, x);
		rngstep( a << 2 , a, b, mm, m, m2, r, x);
		rngstep( a >> 16, a, b, mm, m, m2, r, x);
	}

	ctx->randb = b;
	ctx->randa = a;
}


#define mix(a,b,c,d,e,f,g,h) \
{ \
	a ^= b << 11;  d += a; b += c;	\
	b ^= c >> 2;   e += b; c += d;	\
	c ^= d << 8;   f += c; d += e;	\
	d ^= e >> 16;  g += d; e += f;	\
	e ^= f << 10;  h += e; f += g;	\
	f ^= g >> 4;   a += f; g += h;	\
	g ^= h << 8;   b += g; h += a;	\
	h ^= a >> 9;   c += h; a += b;	\
}

/*
 *	Initialize the random context with a seed.  If there's no
 *	seed, just use all zeroes.
 */
void isaac_randinit(randctx *ctx, void const *seed, int seedlen)
{
	int i;
	uint32_t a,b,c,d,e,f,g,h;
	uint32_t *m,*r;

	memset(ctx, 0, sizeof(*ctx));

	if (seed && (seedlen > 0)) {
		if (seedlen > sizeof(ctx->randrsl)) seedlen = sizeof(ctx->randrsl);

		memcpy(ctx->randrsl, seed, seedlen);
	}

	m = ctx->randmem;
	r = ctx->randrsl;
	a = b = c = d = e = f= g = h = 0x9e3779b9;  /* the golden ratio */

	for (i = 0; i < 4; ++i) {          /* scramble it */
		mix(a,b,c,d,e,f,g,h);
	}

	/* initialize using the contents of r[] as the seed */
	for (i = 0; i < RANDSIZ; i += 8) {
		a += r[i  ]; b += r[i+1];
		c += r[i+2]; d += r[i+3];
		e += r[i+4]; f += r[i+5];
		g += r[i+6]; h += r[i+7];

		mix(a,b,c,d,e,f,g,h);

		m[i  ] = a; m[i+1] = b; m[i+2] = c; m[i+3] = d;
		m[i+4] = e; m[i+5] = f; m[i+6] = g; m[i+7] = h;
	}

	/* do a second pass to make all of the seed affect all of m */
	for (i = 0; i < RANDSIZ; i += 8) {
		a += m[i  ]; b += m[i+1];
		c += m[i+2]; d += m[i+3];
		e += m[i+4]; f += m[i+5];
		g += m[i+6]; h += m[i+7];

		mix(a,b,c,d,e,f,g,h);

		m[i  ] = a; m[i+1] = b; m[i+2] = c; m[i+3] = d;
		m[i+4] = e; m[i+5] = f; m[i+6] = g; m[i+7] = h;
	}

	isaac(ctx);            /* fill in the first set of results */
	ctx->randcnt = RANDSIZ;  /* prepare to use the first set of results */
}

uint32_t isaac_rand(randctx *ctx)
{
	uint32_t r;

	r = ctx->randcnt--;
	if (ctx->randcnt > 0) return r;

	isaac(ctx);
	ctx->randcnt = RANDSIZ - 1;
	return ctx->randrsl[RANDSIZ - 1];
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
			printf("%.8x",ctx.randrsl[j]);

			if ((j&7) == 7) printf("\n");
		}
	}
}
#endif

