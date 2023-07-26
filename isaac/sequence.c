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
 *	Modified for BFD by Alan DeKok.  Modification placed in the public domain.
 */
#include "sequence.h"

#define ind(x)  (start[(x >> 2) & (RANDSIZ - 1)])
#define rngstep(mix,a,b,m,m2,r) \
{ \
	uint32_t y, x = *m;		        \
	a = ((a^(mix)) + *(m2++));              \
	*(m++) = y = (ind(x) + a + b);	        \
	*(r++) = b = (ind(y >> RANDSIZL) + x) ;	\
}

static void isaac(bfd_isaac_randctx *ctx)
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
	ctx->index = 0;
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

#define to_network(_array, _num) \
	do { \
		_array[0] = (_num >> 24) & 0xff; \
		_array[1] = (_num >> 16) & 0xff; \
		_array[2] = (_num >> 8) & 0xff; \
		_array[3] = _num & 0xff; \
	} while (0)

#define from_network(_p) ((((uint32_t) (_p)[0]) << 24) | (((uint32_t) (_p)[1]) << 16) | (((uint32_t) (_p)[2]) << 8) | ((uint32_t) (_p)[3]))

/*
 *	Initialize the random context with a seed.  If there's no
 *	seed, just use all zeroes.
 */
void bfd_isaac_init(bfd_isaac_ctx *ctx,  uint32_t seed, uint32_t discriminator, uint8_t const *key, size_t keylen)
{
	int i, j;
	uint32_t a[8];
	uint32_t *m,*r;
	bfd_isaac_randctx *randctx = &ctx->randctx[0];

	memset(ctx, 0, sizeof(*ctx));

	if (key && (keylen > 0)) {
		uint8_t *p = (uint8_t *) &randctx->page[0];

		if (keylen > (sizeof(randctx->page) - 4)) keylen = sizeof(randctx->page) - 8;

		/*
		 *	Endian-ness matters.
		 */
		to_network(p, seed);
		p += 4;
		to_network(p, discriminator);
		p += 4;

		memcpy(p, key, keylen);
	}

	m = randctx->pool;
	r = randctx->page;

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

	isaac(randctx);		/* fill in the first set of results */

	randctx->index = 0;		/* prepare to use the first set of results */

	/*
	 *	All sequences start at zero.
	 */
	ctx->sequence = 0;
}

/*
 *	Churn the pool if necessary.  The various function which get
 *	the number churn the pool *before* getting the numbers.
 *
 *	Churning the pool may take significant amounts of CPU time, so
 *	we expose an API which lets the caller decide to churn the
 *	pool.
 */
void bfd_isaac_churn(bfd_isaac_ctx *ctx)
{
	bfd_isaac_randctx *randctx = &ctx->randctx[ctx->active];

	if (ctx->next_page_valid) return;

	/*
	 *	Copy the current context to the other page, and run
	 *	ISAAC on it.
	 */
	ctx->randctx[ctx->active ^ 0x01] = *randctx;
	ctx->next_page_valid = 1;

	randctx = &ctx->randctx[ctx->active ^ 0x01];
	isaac(randctx);
}

/*
 *	Get the next sequence number, and the next ISAAC output.
 */
void bfd_isaac_sequence_next(bfd_isaac_ctx *ctx, uint8_t *sequence, uint8_t *auth_key)
{
	bfd_isaac_randctx *randctx = &ctx->randctx[ctx->active];
	uint32_t r;

	/*
	 *	Have we used all of the numbers in this page?
	 */
	if (randctx->index > 256) {
		/*
		 *	We may already have a second page available.
		 *	If so, use that.
		 */
		if (ctx->next_page_valid) {
			ctx->active ^= 0x01;
			randctx = &ctx->randctx[ctx->active];
		} else {
			isaac(randctx);
		}

		ctx->next_page_valid = 0;
	}

	r = randctx->page[randctx->index++];

	to_network(auth_key, r);
	
	r = ctx->sequence++;
	to_network(sequence, r);
}

#define FNV_MAGIC_INIT (0x811c9dc5)
#define FNV_MAGIC_PRIME (0x01000193)

/*
 *	FNV-1A from:
 *
 *	http://www.isthe.com/chongo/tech/comp/fnv/
 */
static uint32_t fnv1a_update(const uint8_t *data, size_t datalen, uint32_t hash)
{
	uint8_t const *end = data + datalen;

	if (datalen == 0) return hash;

	while (data < end) {
		hash ^= (uint32_t) (*data++);

		/*
		 *	The multiply can be replaced with
		 *
		 *	hash += (hash<<1) + (hash<<4) + (hash<<7) + (hash<<8) + (hash<<24);
		 */
		hash *= FNV_MAGIC_PRIME;
	}

	return hash;
}

#define fnv1a(_a, _b) fnv1a_update(_a, _b, FNV_MAGIC_INIT)

/*
 *	Calculate the FNV-1A hash over the (R + packet + R).
 */
void bfd_isaac_fnv1a_next(bfd_isaac_ctx *ctx, uint32_t *sequence, uint32_t *digest, uint8_t const *data, size_t datalen)
{
	bfd_isaac_randctx *randctx = &ctx->randctx[ctx->active];
	uint8_t	array[4];
	uint32_t r, hash;

	if (randctx->index > 256) {
		isaac(randctx);
		ctx->next_page_valid = 0;
	}

	*digest = 0;
	r = ctx->sequence++;
	to_network(((uint8_t *) sequence), r);
	
	r = randctx->page[randctx->index++];
	to_network(array, r);

	hash = fnv1a(&array[0], 4);
	hash = fnv1a_update(data, datalen, hash);
	hash = fnv1a_update(&array[0], 4, hash);

	to_network(((uint8_t *) digest), hash);
}

static int sequence_offset(bfd_isaac_ctx *ctx, uint32_t sequence)
{
	bfd_isaac_randctx *randctx = &ctx->randctx[ctx->active];
	uint32_t offset;

	/*
	 *	Check for rollover at 2^32
	 */
	if (sequence < ctx->sequence) {
		/*
		 *	If we're at 2^32-1, then the other sequence
		 *	can't be more than 512 off that.
		 */
		if (sequence > 511) return -1;

		/*
		 *	And we can't be more than 512 from rolling
		 *	over.
		 */
		offset = (~(uint32_t) 0) - ctx->sequence + 1;
		if (offset > 511) return -1;

		/*
		 *	Now we know that this addition won't roll over
		 *	either, as both inputs are 0..511.
		 */
		offset += sequence;

	} else if (sequence == ctx->sequence) {
		/*
		 *	Can't re-use sequence numbers.
		 */
		return -1;

	} else {
		offset = sequence - ctx->sequence;
	}

	/*
	 *	Add in the current index, 0..256.
	 */
	offset += randctx->index;

	/*
	 *	Each page is 256 numbers.  We have 256 "free" numbers
	 *	in the next page, and 256 numbers in this page, minus
	 *	how many numbers we've used from this page.
	 */
	if (offset > 511) return -1;

	return offset;
}

static int get_checked_rand(bfd_isaac_ctx *ctx, uint32_t offset, uint32_t *output)
{
	bfd_isaac_randctx *randctx = &ctx->randctx[ctx->active];
	
	/*
	 *	We're within the current page, just check it.
	 */
	if (offset < 256) {
		*output = randctx->page[offset];
		return 0;
	}

	/*
	 *	We try to avoid re-calculating things as much as
	 *	possible.  This means that if we calculate the "next"
	 *	page, we cache it so that we don't need to calculate
	 *	it more than once.
	 *
	 *	If we've already copied over the current page to the
	 *	inactive one, then use use the inactive one.  It's the
	 *	next page in the sequence.
	 */
	if (!ctx->next_page_valid) {
		ctx->randctx[ctx->active ^ 0x01] = *randctx;
		ctx->next_page_valid = 1;
	}

	offset -= 256;
	*output = randctx->page[offset];
	return 1;
}

static int swap_to_page(bfd_isaac_ctx *ctx, int offset, int next_page)
{
	bfd_isaac_randctx *randctx = &ctx->randctx[ctx->active];

	/*
	 *	Might as well check for crazy things.
	 */
	if ((offset < 0) || (offset > 255)) return -3;

	/*
	 *	This new page is OK, so we swap to using it.
	 */
	if (next_page) {
		ctx->active ^= 0x01;
		randctx = &ctx->randctx[ctx->active];

		ctx->next_page_valid = 0;	
	}

	/*
	 *	We've used this index and this sequence number.  Go to
	 *	the next one.
	 */
	offset++;
	ctx->sequence += offset;
	randctx->index = offset;
	return 0;
}

int bfd_isaac_sequence_check(bfd_isaac_ctx *ctx, uint8_t const *sequence, uint8_t const *auth_key)
{
	bfd_isaac_randctx *randctx = &ctx->randctx[ctx->active];
	int offset, next_page;
	uint32_t expected;

	offset = sequence_offset(ctx, from_network(sequence));
	if (offset < 0) return -1;

	next_page = get_checked_rand(ctx, offset, &expected);
	if (expected != from_network(auth_key)) {
		return -2;
	}

	return swap_to_page(ctx, offset, next_page);
}

int bfd_isaac_fnv1a_check(bfd_isaac_ctx *ctx, uint8_t const *sequence, uint8_t *digest, uint8_t const *data, size_t datalen)
{
	bfd_isaac_randctx *randctx = &ctx->randctx[ctx->active];
	int offset, next_page;
	uint8_t array[4];
	uint32_t r, hash, expected;
	
	offset = sequence_offset(ctx, from_network(sequence));
	if (offset < 0) return -1;

	next_page = get_checked_rand(ctx, offset, &r);

	to_network(array, r);

	/*
	 *	Remember what's in the packet, then calculate our hash.
	 */
	expected = from_network(digest);
	memset(digest, 0, 4);

	hash = fnv1a(&array[0], 4);
	hash = fnv1a_update(data, datalen, hash);
	hash = fnv1a_update(&array[0], 4, hash);

	memcpy(digest, &expected, 4);

	if (hash != expected) {
		return -2;
	}

	return swap_to_page(ctx, offset, next_page);
}

#ifdef NEVER
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <limits.h>

int main(int argc, char const *argv[])
{
	int i;

	bfd_isaac_ctx ctx;

	uint32_t seed = 0x0bfd5eed; /* bfd seed */
	uint32_t your_discriminator = 0x4002d15c;
	char const *key = "RFC5880June";

	uint32_t sequence, auth_key;
	uint8_t nbo_sequence[4];
	uint8_t nbo_auth_key[4];

	if (argc >= 2) {
		key = argv[1];
	}

	if (argc >= 3) {
		seed = strtoul(argv[2], NULL, 16);
	}

	if (argc >= 4) {
		your_discriminator = strtoul(argv[3], NULL, 16);
	}

	bfd_isaac_init(&ctx, seed, your_discriminator, (uint8_t const *) key, strlen(key));

	memset(nbo_sequence, 0, sizeof(nbo_sequence));
	memset(nbo_auth_key, 0, sizeof(nbo_auth_key));

	printf("Seed\t0x%08x\n", seed);
	printf("Y-Disc\t0x%08x\n", your_discriminator);
	printf("Key\t%s\n\n", key);

	for (i = 0; i < 8; i++) {
		bfd_isaac_sequence_next(&ctx, nbo_sequence, nbo_auth_key);

		sequence = from_network(nbo_sequence);
		auth_key = from_network(nbo_auth_key);

		printf("%08x %08x\n", sequence, auth_key);
	}


	return 0;
}
#endif
