#include <stdint.h>
#include <stdio.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

static const unsigned char key_data[] = {
    'T', 'h', 'e', 'Q', 'u', 'i', 'c', 'k',
    'B', 'r', 'o', 'w', 'n', 'F', 'o', 'x'
};

#define ROUNDS 1000000

int main(int argc, char *argv[])
{
    EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
    uint32_t *in_buf, *out_buf, *buf1, *buf2;
    int bufsize = 256 * sizeof(uint32_t), outsize, flen;
    struct timeval start, end;
    long micros_used;

    uint8_t key[32], iv[32];
    int nrounds = 5;
    int i;

    buf1 = calloc(256, sizeof(uint32_t));
    buf2 = calloc(256, sizeof(uint32_t));

    i = EVP_BytesToKey(EVP_aes_128_ofb(), EVP_sha1(), NULL /* no salt */, key_data, sizeof(key_data), 5, key, iv);
    assert(i == 16);

    EVP_CIPHER_CTX_init(ctx);
    EVP_EncryptInit_ex(ctx, EVP_aes_128_ofb(), NULL, key, iv);

    gettimeofday(&start, NULL);
    for (i = 0; i < ROUNDS; i++) {
	if (i % 2 == 0) {
	    in_buf = buf1; out_buf = buf2;
	} else {
	    in_buf = buf2; out_buf = buf1;
	}
	outsize = bufsize;
	assert(EVP_EncryptUpdate(ctx, (uint8_t *)out_buf, &outsize, (uint8_t *)in_buf, bufsize) != 0);

#if 0
        /* Check the output for consistency between runs of the program for first three rounds. */
	if (i <= 3) {
	    int j;
	    for (j = 0; j < 256; j++) {
	        fprintf(stderr, "%08x ", out_buf[j]);
		if (j % 8 == 7) {
		    fprintf(stderr, "\n");
		}
	    }
	    fprintf(stderr, "\n");
	}
#endif

#if 0
	assert(outsize == bufsize);
	EVP_EncryptFinal_ex(ctx, (uint8_t *)out_buf + outsize, &flen);
	assert(flen == 0);
#endif
    }
    gettimeofday(&end, NULL);

    micros_used = end.tv_sec - start.tv_sec;
    micros_used = ((micros_used * 1000000) + end.tv_usec) - start.tv_usec;
    printf("%d rounds uses %ld micro-seconds\n", ROUNDS, micros_used);
}
