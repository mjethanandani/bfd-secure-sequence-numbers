# BFD API

The BFD API is composed of two files:

* `sequence.h`, which contains the data structures and function prototypes.

* `sequence.c` which contains the API implementation

The various `ctx` structures are public in order to allow the caller
to know the correct size for allocations.  The fields of the data
structures _should not_ be modified, or the functions will crash.

The code does not do any memory allocations.  The only dependency on
external functions is `memset()` and `memcpy()`.  As a result, it
should be fairly easy to port it to any other systems.

These funtions are NOT thread-safe.

The functions assume that they are operating on a packet which is
stored as an array of `uint8_t` bytes.  That is, on send, the caller should
generate the packet contents via any means that is desired, and then
call these routines on the "raw" packet data.

Similarly, on receive, the caller should validate that the BFD packet
is OK, and then call these functions with the pointers directly to the
"raw" fields of the packet.

## Initialization

`void bfd_isaac_init(bfd_isaac_ctx *ctx, uint32_t sequence, uint32_t seed, uint8_t const *key, size_t keylen)`

This function initializes the `ctx`, at a particular `sequence`
number, using a `seed`, with a secret `key`.  The maximum `keylen` is
1020 bytes.  Any `key` data after that will be ignored.

The `sequence` and `seed` values should be in host byte order.

There are no restrictions on `ctx`.  It can be dynamically allocated
by the caller, or statically allocated.

There are no restrictions on `key`.  The data can be anything.

ISAAC will generate random numbers in 256-entry "pages".  It is
essentially zero cost to access a number within a particular page.

### Generating New Pages

It can be expensive to generate a new "page" of numbers.  All of the
functions below will automatically generate a new page if necessary.
However, it can be useful to control when this generation occurs.
There is therefore a function to create new pages:

`void bfd_isaac_churn(bfd_isaac_ctx *ctx)`

This function calculates the next "page" for ISAAC in the `ctx`, if
necessary.  If no new page is needed, this function does nothing.
Otherwise, it calculates a new page.

Precalculating a page means that the functions below will nearly
always take a small, bounded amount of time to execute.

This function allows the caller to separate the slow ISAAC "new page"
function from the fast ISAAC "get new number from page" function.
That is, `bfd_isaac_churn()` can be called from the "slow path", where
timing constraints are not an issue.

## Meticulous Keyed ISAAC API

Once the `ctx` has been initialized, it can be used to send and
receive packets.

### Sending packets

`void bfd_isaac_sequence_next(bfd_isaac_ctx *ctx, uint8_t *sequence, uint8_t *auth_key)`

This function takes a `ctx`, and stores the next `sequence` number,
and the ISAAC `auth_key`.

The internal sequence number is automatically incremented every time
this function is called.  The sequence number automatically wraps at
2^32.

The `sequence` and `auth_key` variables should point to the relevant
fields in the Auth Type structure of a BFD packet.  There should be
room to write four (4) octets of data.  The values are written in
network byte order.

The function cannot fail, and returns nothing.

### Receiving Packets

`int bfd_isaac_sequence_check(bfd_isaac_ctx *ctx, uint8_t const *sequence, uint8_t const *auth_key)`

This function takes a `ctx`, and checks if the `sequence` and
`auth_key` match what is expected.

The `sequence` and `auth_key` variables should point to the relevant
fields in the Auth Type structure of a BFD packet.  There should be
room to read four (4) octets of data.  The values should be in network
byte order, as received from the network.

Note that this function does not perform any checks that the packet is
a correctly formed BFD packet, or that the `Auth Type` value and
format matches that of Meticulous Keyed ISAAC.  The application still
must perform all of those checks before calling this function.

The function returns:

* `0` for success 
* `-1` when the `sequence` number is out of bounds
* `-2` when the supplied `auth_key` does not match the expected ISAAC output for that `sequence` number.

Due to the limitations of the current implementation, this API is
guaranteed to handle at least 256 lost packets, but not more than
that.  In some cases, it can handle 511 lost packets, but those cases
are rare.

## Meticulous Keyed FNV1A API

Once the `ctx` has been initialized, it can be used to send and
receive packets.

### Sending packets

`void bfd_isaac_fnv1a_next(bfd_isaac_ctx *ctx, uint8_t *sequence, uint8_t *digest, uint8_t const *packet, size_t packetlen)`

This function takes a `ctx`, and stores the `digest` at the given location.

The `sequence` and `digest` variables should point to the relevant
fields in the Auth Type structure of a BFD packet.  There should be
room to write four (4) octets of data.  The values are written in
network byte order.

The `digest` is set to `0`, the hash calculated, and then the results
stored in the `digest` location in network byte order.

The sequence number is automatically incremented every time this
function is called.  The sequence number automatically wraps at
2^32.

The hash is calculated using the _network byte order_ of the ISAAC
output.  This is so that the calculation is the same for any machine,
no matter what the host byte order.

The function cannot fail, and returns nothing.

### Receiving Packets

`int bfd_isaac_fnv1a_check(bfd_isaac_ctx *ctx, uint8_t const *sequence, uint32_t *digest, uint8_t const *packet, size_t packetlen)`

This function takes a `ctx`, and checks if the `sequence` and `digest`
match what is expected.  Note that this function does not perform any
checks that the `packet` is a correctly formed BFD packet, or that the
`Auth Type` value and format matches that of Meticulous Keyed FNV1A.
The application still must perform all of those checks before calling
this function.

The `sequence` and `digest` variables should point to the relevant
fields in the Auth Type structure of a BFD packet.  There should be
room to read four (4) octets of data.  The values should be in network
byte order, as received from the network.

The check saves the `digest` value, sets the `digest` value to zero,
calculates the expected hash for the particular `sequence`, and then
compares that calculated value to the saved `digest` value.

The function returns:

* `0` for success 
* `-1` when the `sequence` number is out of bounds
* `-2` when the supplied `digest` does not match the expected hash output for that `sequence` number.

Due to the limitations of the current implementation, this API is
guaranteed to handle at least 256 lost packets, but not more than
that.  In some cases, it can handle 511 lost packets, but those cases
are rare.
