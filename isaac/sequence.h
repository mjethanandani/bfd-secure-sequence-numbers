/*
 *	sequence.h
 */

#define RANDSIZL   (8)
#define RANDSIZ    (1 << RANDSIZL)

#include <stdint.h>
#include <string.h>

/*
 *	Context of random number generator
 **/
typedef struct bfd_isaac_randctx {
	uint32_t index;				// index of used numbers in the page
	uint32_t randa;
	uint32_t randb;
	uint32_t num_pages;			// number of pages we have generated.

	uint32_t page[RANDSIZ];			// current page of 256 random numbers to output
	uint32_t pool[RANDSIZ];			// pool of secret entropy
} bfd_isaac_randctx;

typedef struct bfd_isaac_ctx {
	uint32_t sequence;			// sequence numbers to track

	int	active;				// which radctx is active
	int	next_page_valid;		// is the inactive context the "next" page?

	bfd_isaac_randctx	randctx[2];
} bfd_isaac_ctx;

void bfd_isaac_init(bfd_isaac_ctx *ctx, uint32_t sequence, uint32_t seed, uint8_t const *key, size_t keylen);

void bfd_isaac_churn(bfd_isaac_ctx *ctx);

void bfd_isaac_sequence_next(bfd_isaac_ctx *ctx, uint8_t *sequence, uint8_t *auth_key);

int bfd_isaac_sequence_check(bfd_isaac_ctx *ctx, uint8_t const *sequence, uint8_t const *auth_key);

void bfd_isaac_fnv1a_next(bfd_isaac_ctx *ctx, uint32_t *sequence, uint32_t *digest, uint8_t const *packet, size_t packetlen);

int bfd_isaac_fnv1a_check(bfd_isaac_ctx *ctx, uint8_t const *sequence, uint8_t *digest, uint8_t const *packet, size_t packetlen);
