#include <stdio.h>
#include <assert.h>
#define HEX_BITS 4
#define HEX_STRING_LENGTH ((2 * sizeof(char *)) + 3)
#define PTR_SIZE sizeof(char *)
void ptr_to_hex_string(void *ptr, char *out_str);
void main()
{
  char *haha = "haha";
  char outputstr[HEX_STRING_LENGTH];
  setbuf(stdout, NULL);
  ptr_to_hex_string(haha, outputstr);
  printf("%s\n", outputstr);
  printf("%p\n", haha);
}

void ptr_to_hex_string(void *ptr, char *out_str)
{
  const char *hex = "0123456789ABCDEF";
  char *ptr_ptr = (char *)&ptr + PTR_SIZE - 1;
  // write 0x**** prefix to output string
  out_str[0] = '0';
  out_str[1] = 'X';
  int out_str_index = 2;
  int is_begining = 1;
  // Using double pointer to format hex string
  for (int i = 0; i < PTR_SIZE; i++)
  {
    if (is_begining)
    {
      if (*ptr_ptr == 0)
      {
        ptr_ptr--;
        continue;
      }
      else
      {
        is_begining = 0;
      }
    }

    out_str[out_str_index] = hex[(*ptr_ptr >> HEX_BITS) & 0xF];
    out_str[out_str_index + 1] = hex[*ptr_ptr & 0xF];
    out_str_index += 2;
    ptr_ptr--;
  }
  out_str[out_str_index] = '\0';
}
