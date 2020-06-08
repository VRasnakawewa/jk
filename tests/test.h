#ifndef TEST_H
#define TEST_H

static int testPassed = 0;
static int testFailed = 0;

#define CHECK(condition) if (!condition) return __LINE__

static void run(int (*fn)(), const char *name)
{
    int r = fn();
    if (r) printf("FAILED: %s (at line %d)\n", name, r);
    if (r) testFailed++; else testPassed++;
}

#endif
