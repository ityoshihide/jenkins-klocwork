#include <stdio.h>
#include <string.h>


/* --- Added examples to increase buffer overflow findings (build-safe) --- */

/* strcpy overflow: destination too small */
void buffer_overflow_via_strcpy(void)
{
    char small[8];
    /* definitely longer than 8 incl. null terminator */
    strcpy(small, "0123456789ABCDEF");
    printf("%s\n", small);
}

/* strcat overflow: concatenation exceeds destination size */
void buffer_overflow_via_strcat(void)
{
    char buf[8] = "ABC";
    strcat(buf, "DEFGHIJK"); /* overflow */
    printf("%s\n", buf);
}

/* memcpy overflow: copying more than destination capacity */
void buffer_overflow_via_memcpy(void)
{
    char dst[8];
    char src[24];

    memset(src, 'A', sizeof(src));
    src[sizeof(src) - 1] = '\0';

    memcpy(dst, src, 16); /* overflow: dst is 8 bytes */
    printf("%c\n", dst[0]);
}

/* Index overflow: write past end of fixed array */
void buffer_overflow_via_index(void)
{
    char a[4] = {0};
    a[10] = 1; /* out-of-bounds write */
    printf("%d\n", a[0]);
}

/* Invoke newly added cases so analysis sees reachable code paths */
void invoke_more_overflows(void)
{
    buffer_overflow_via_strcpy();
    buffer_overflow_via_strcat();
    buffer_overflow_via_memcpy();
    buffer_overflow_via_index();
}
