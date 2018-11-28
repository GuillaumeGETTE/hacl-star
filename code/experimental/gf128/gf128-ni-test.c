#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

typedef uint64_t cycles;

static __inline__ cycles cpucycles_begin(void)
{
  uint64_t rax,rdx,aux;
  asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx), "=c" (aux) : : );
  return (rdx << 32) + rax;
  //  unsigned hi, lo;
  //__asm__ __volatile__ ("CPUID\n\t"  "RDTSC\n\t"  "mov %%edx, %0\n\t"  "mov %%eax, %1\n\t": "=r" (hi), "=r" (lo):: "%rax", "%rbx", "%rcx", "%rdx");
  //return ( (uint64_t)lo)|( ((uint64_t)hi)<<32 );
}

static __inline__ cycles cpucycles_end(void)
{
  uint64_t rax,rdx,aux;
  asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx), "=c" (aux) : : );
  return (rdx << 32) + rax;
  //  unsigned hi, lo;
  //__asm__ __volatile__ ("RDTSCP\n\t"  "mov %%edx, %0\n\t"  "mov %%eax, %1\n\t"  "CPUID\n\t": "=r" (hi), "=r" (lo)::     "%rax", "%rbx", "%rcx", "%rdx");
  //return ( (uint64_t)lo)|( ((uint64_t)hi)<<32 );
}

extern void Hacl_Gf128_NI_ghash(uint8_t* out, int in_len, uint8_t* in, uint8_t* k);

#define ROUNDS 100000
#define SIZE   16384

int main() {
  int in_len = 92;
  uint8_t in[92] = {
    0xfe,0xed,0xfa,0xce,0xde,0xad,0xbe,0xef,
    0xfe,0xed,0xfa,0xce,0xde,0xad,0xbe,0xef,
    0xab,0xad,0xda,0xd2,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x5a,0x8d,0xef,0x2f,0x0c,0x9e,0x53,0xf1,
    0xf7,0x5d,0x78,0x53,0x65,0x9e,0x2a,0x20,
    0xee,0xb2,0xb2,0x2a,0xaf,0xde,0x64,0x19,
    0xa0,0x58,0xab,0x4f,0x6f,0x74,0x6b,0xf4,
    0x0f,0xc0,0xc3,0xb7,0x80,0xf2,0x44,0x45,
    0x2d,0xa3,0xeb,0xf1,0xc5,0xd8,0x2c,0xde,
    0xa2,0x41,0x89,0x97,0x20,0x0e,0xf8,0x2e,
    0x44,0xae,0x7e,0x3f};
  uint8_t key[16] = {
    0xac,0xbe,0xf2,0x05,0x79,0xb4,0xb8,0xeb,
    0xce,0x88,0x9b,0xac,0x87,0x32,0xda,0xd7};
  uint8_t exp[16] = {
    0xcc,0x9a,0xe9,0x17,0x57,0x29,0xa6,0x49,
    0x93,0x6e,0x89,0x0b,0xd9,0x71,0xa8,0xbf};
  uint8_t comp[16] = {0};
  bool ok = true;

  Hacl_Gf128_NI_ghash(comp,92,in,key);
  printf("GF128 NI Result:\n");
  printf("computed:");
  for (int i = 0; i < 16; i++)
    printf("%02x",comp[i]);
  printf("\n");
  printf("expected:");
  for (int i = 0; i < 16; i++)
    printf("%02x",exp[i]);
  printf("\n");
  ok = true;
  for (int i = 0; i < 16; i++)
    ok = ok & (exp[i] == comp[i]);
  if (ok) printf("Success!\n");

  uint8_t plain[SIZE];
  uint64_t res = 0;
  uint8_t tag[16];
  cycles a,b;
  clock_t t1,t2;
  uint64_t count = ROUNDS * SIZE;

  memset(plain,'P',SIZE);
  memset(key,'K',16);
  for (int j = 0; j < ROUNDS; j++) {
    Hacl_Gf128_NI_ghash(plain,SIZE,plain,key);
  }

  t1 = clock();
  a = cpucycles_begin();
  for (int j = 0; j < ROUNDS; j++) {
    Hacl_Gf128_NI_ghash(tag,SIZE,plain,key);
    res ^= tag[0] ^ tag[15];
  }
  b = cpucycles_end();
  t2 = clock();
  clock_t tdiff2 = t2 - t1;
  cycles cdiff2 = b - a;

  printf("GF128-NI PERF:\n");
  printf("cycles for %" PRIu64 " bytes: %" PRIu64 " (%.2fcycles/byte)\n",count,(uint64_t)cdiff2,(double)cdiff2/count);
  printf("time for %" PRIu64 " bytes: %" PRIu64 " (%.2fus/byte)\n",count,(uint64_t)tdiff2,(double)tdiff2/count);
  printf("bw %8.2f MB/s\n",(double)count/(((double)tdiff2 / CLOCKS_PER_SEC) * 1000000.0));

}
