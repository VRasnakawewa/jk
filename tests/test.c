#include <stdio.h>

#include "test.h"
#include "../jstr.h"
#include "../map.h"

int testCmpJstrFunc()
{
    jstr j1, j2, j3;
    jstr j4, j5, j6;

    // ASCII
    // -----
    // char *, char *
    CHECK(cmpJstr("cat","cat") == 0);
    CHECK(cmpJstr("cat","run") != 0);
    CHECK(cmpJstr("cat","catcat") != 0);
    // char *, jstr
    CHECK(cmpJstr("cat", j1 = newJstr("cat",0)) == 0);
    CHECK(cmpJstr("cat", j2 = newJstr("run",0)) != 0);
    CHECK(cmpJstr("cat", j3 = newJstr("catcat",0)) != 0);
    destroyJstr(j1);
    destroyJstr(j2);
    destroyJstr(j3);
    // jstr, char *
    CHECK(cmpJstr(j1 = newJstr("cat",0), "cat") == 0);
    CHECK(cmpJstr(j2 = newJstr("run",0), "cat") != 0);
    CHECK(cmpJstr(j3 = newJstr("catcat",0), "cat") != 0);
    destroyJstr(j1);
    destroyJstr(j2);
    destroyJstr(j3);
    // jstr, jstr
    CHECK(cmpJstr(j1 = newJstr("cat",0), j4 = newJstr("cat",0)) == 0);
    CHECK(cmpJstr(j2 = newJstr("run",0), j5 = newJstr("cat",0)) != 0);
    CHECK(cmpJstr(j3 = newJstr("catcat",0), j6 = newJstr("cat",0)) != 0);
    destroyJstr(j1);
    destroyJstr(j2);
    destroyJstr(j3);
    destroyJstr(j4);
    destroyJstr(j5);
    destroyJstr(j6);

    // UTF-8
    // -----
    CHECK(cmpJstr(j1 = newJstr("Σὲ γνωρίζω ἀπὸ τὴν κόψη", 48), j2 = newJstr("Σὲ γνωρίζω ἀπὸ τὴν κόψη", 48)) == 0);
    CHECK(cmpJstr(j3 = newJstr("Σὲ γνωρίζω ἀπὸ τὴν κόψη", 48), j4 = newJstr("⡌⠁⠧⠑ ⠼⠁⠒", 22)) != 0);
    destroyJstr(j1);
    destroyJstr(j2);
    destroyJstr(j3);
    destroyJstr(j4);

    return 0;
}

int main(int argc, char **argv)
{
    run(testCmpJstrFunc, "Test cmpJstr function");
    printf("\n"
        "PASSED: %d\n"
        "FAILED: %d\n", testPassed, testFailed);
    return 0;
}
