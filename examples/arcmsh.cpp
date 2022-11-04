/*
 * arcmsh.c
 * Copyright  : Kyle Harper
 * License    : Follows same licensing as the lz4.c/lz4.h program at any given time.  Currently, BSD 2.
 * Description: Example program to demonstrate the basic usage of the compress/decompress functions within lz4.c/lz4.h.
 *              The functions you'll likely want are LZ4_compress_default and LZ4_decompress_safe.
 *              Both of these are documented in the lz4.h header file; I recommend reading them.
 */

/* Dependencies */
#include <stdio.h>   // For printf()
#include <string.h>  // For memcmp()
#include <stdlib.h>  // For exit()
#include "lz4.h"     // This is all that is required to expose the prototypes for basic compression and decompression.
#include <vector>

/*
 * Simple show-error-and-bail function.
 */
void run_screaming(const char* message, const int code) {
  printf("%s \n", message);
  exit(code);
}



/*
 * main
 */
int main(void) {
  /* Introduction */
  // Below we will have a Compression and Decompression section to demonstrate.
  // There are a few important notes before we start:
  //   1) The return codes of LZ4_ functions are important.
  //      Read lz4.h if you're unsure what a given code means.
  //   2) LZ4 uses char* pointers in all LZ4_ functions.
  //      This is baked into the API and not going to change, for consistency.
  //      If your program uses different pointer types,
  //      you may need to do some casting or set the right -Wno compiler flags to ignore those warnings (e.g.: -Wno-pointer-sign).

  /* Compression */
  // We'll store some text into a variable pointed to by *src to be compressed later.
  int num_elems = 7;
  int64_t* src_int = (int64_t*) malloc(num_elems * sizeof(int64_t));
  src_int[0] = 123123124;
  src_int[1] = 334234;
  src_int[2] = 454365346;
  src_int[3] = 23123123;
  src_int[4] = 3423423;
  src_int[5] = 123123123;
  src_int[6] = 5454552342;
  for (int i = 0; i < num_elems; i++)
    printf("src_int[%d] = %ld\n", i, src_int[i]);
  // const char* const src = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Lorem ipsum dolor site amat.";
  // The compression function needs to know how many bytes exist.  Since we're using a string, we can use strlen() + 1 (for \0).
  // const int src_size = (int)(strlen(src) + 1);
  const int src_size = num_elems * sizeof(int64_t);
  printf("src_size in bytes = %d\n", src_size);
  // LZ4 provides a function that will tell you the maximum size of compressed output based on input data via LZ4_compressBound().
  // const int max_dst_size = LZ4_compressBound(src_size);
  const int max_dst_size = LZ4_compressBound(src_size);
  printf("max_dst_size in bytes = %d\n", max_dst_size);
  // We will use that size for our destination boundary when allocating space.
  // char* compressed_data = (char*) malloc((size_t)max_dst_size);
  int64_t* compressed_data = (int64_t*) malloc((size_t)max_dst_size);
  if (compressed_data == NULL)
    run_screaming("Failed to allocate memory for *compressed_data.", 1);
  // That's all the information and preparation LZ4 needs to compress *src into* compressed_data.
  // Invoke LZ4_compress_default now with our size values and pointers to our memory locations.
  // Save the return value for error checking.
  const int compressed_data_size = LZ4_compress_default((const char*) src_int, (char*) compressed_data, src_size, max_dst_size);
  printf("compressed_data_size in bytes = %d\n", compressed_data_size);
  // Check return_value to determine what happened.
  if (compressed_data_size <= 0)
    run_screaming("A 0 or negative result from LZ4_compress_default() indicates a failure trying to compress the data. ", 1);
  if (compressed_data_size > 0)
    printf("We successfully compressed some data! Ratio: %.2f\n",
        (float) compressed_data_size/src_size);
  // Not only does a positive return_value mean success, the value returned == the number of bytes required.
  // You can use this to realloc() *compress_data to free up memory, if desired.  We'll do so just to demonstrate the concept.
  // compressed_data = (char *)realloc(compressed_data, (size_t)compressed_data_size);
  compressed_data = (int64_t *) realloc(compressed_data, (size_t)compressed_data_size);
  if (compressed_data == NULL)
    run_screaming("Failed to re-alloc memory for compressed_data.  Sad :(", 1);


  /* Decompression */
  // Now that we've successfully compressed the information from *src to *compressed_data, let's do the opposite!
  // The decompression will need to know the compressed size, and an upper bound of the decompressed size.
  // In this example, we just re-use this information from previous section,
  // but in a real-world scenario, metadata must be transmitted to the decompression side.
  // Each implementation is in charge of this part. Oftentimes, it adds some header of its own.
  // Sometimes, the metadata can be extracted from the local context.

  // First, let's create a *new_src location of size src_size since we know that value.
  // char* const regen_buffer = (char*)malloc(src_size);
  int64_t* const regen_buffer = (int64_t*) malloc(src_size);
  if (regen_buffer == NULL)
    run_screaming("Failed to allocate memory for *regen_buffer.", 1);
  // The LZ4_decompress_safe function needs to know where the compressed data is, how many bytes long it is,
  // where the regen_buffer memory location is, and how large regen_buffer (uncompressed) output will be.
  // Again, save the return_value.
  // const int decompressed_size = LZ4_decompress_safe((char*) compressed_data, (char*) regen_buffer, compressed_data_size, src_size);
  int target_output_size = (num_elems-1) * 8;
  const int decompressed_size = LZ4_decompress_safe_partial((char*) (compressed_data), (char*) regen_buffer, src_size, target_output_size, src_size);
  printf("decompressed_size in bytes = %d\n", decompressed_size);
  // free(compressed_data);   /* no longer useful */
  if (decompressed_size < 0)
    run_screaming("A negative result from LZ4_decompress_safe indicates a failure trying to decompress the data.  See exit code (echo $?) for value returned.", decompressed_size);
  if (decompressed_size >= 0)
    printf("We successfully decompressed some data!\n");
  // Not only does a positive return value mean success,
  // value returned == number of bytes regenerated from compressed_data stream.
  // if (decompressed_size != src_size)
    // run_screaming("Decompressed data is different from original! \n", 1);

  /* Validation */
  // We should be able to compare our original *src with our *new_src and be byte-for-byte identical.
  for (int i = 0; i < num_elems; i++) {
    printf("src_int[%d] = %ld\n", i, src_int[i]);
    printf("regen_buffer[%d] = %ld\n", i, regen_buffer[i]);
  }
  if (memcmp(src_int, regen_buffer, src_size) != 0)
    run_screaming("Validation failed.  *src and *new_src are not identical.", 1);
  printf("Validation done.\n");
  free(src_int);
  free(compressed_data);
  free(regen_buffer);
  return 0;
}
