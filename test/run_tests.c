#include <stdio.h>
#include <stdlib.h>
#include <libesm/aho_corasick.h>
#include <string.h>

ac_error_code
free_string(void *item, void *data)
{
    // Do nnothing
}

char *
test_init(void)
{
    if ( ! ac_index_new()) {
        return "ac_index_new() should not be NULL";
    }
    
    return NULL;
}

ac_error_code
collect_results(void *context, ac_result *result)
{
    char **ctx = (char **) context;
    char *input = *ctx;
    char *object = (char *) result->object;

    if (input) {
        asprintf(ctx, "%s, (%d %d %s)",
            input, result->start, result->end, object);
    } else {
        asprintf(ctx, "(%d %d %s)",
            result->start, result->end, object);
    }

    return AC_SUCCESS;
}

char *
test_query(void)
{
    ac_index *index = ac_index_new();

    if ( ! index) {
        return "Error constructing index.";
    }

    if (ac_index_enter(index, "he", 2, "HE") != AC_SUCCESS) {
        return "Error adding to index.";
    }

    if (ac_index_enter(index, "she", 3, "SHE") != AC_SUCCESS) {
        return "Error adding to index.";
    }

    if (ac_index_enter(index, "his", 3, "HIS") != AC_SUCCESS) {
        return "Error adding to index.";
    }

    if (ac_index_enter(index, "hers", 4, "HERS") != AC_SUCCESS) {
        return "Error adding to index.";
    }
    
    if (ac_index_fix(index) != AC_SUCCESS) {
        return "Error fixing index.";
    }

    char *phrase = "this here is history";
    //              .123456789.123456789
    //               --- --      ---
    int phrase_length = strlen(phrase);

    char *results = NULL; 

    if (ac_index_query_cb(index,
                          phrase,
                          phrase_length,
                          collect_results,
                          &results) != AC_SUCCESS) {
        return "Error running query.";
    }

    if ( ! results) {
        return "No results returned.";
    }

    const char *target = "(1 4 HIS), (5 7 HE), (13 16 HIS)";
    if (strncmp(results, target, strlen(target)) != 0) {
        char *message = NULL;
        asprintf(&message, "Expected %s but got %s", target, results);
        return message;
    }

    
    if (ac_index_free(index, free_string) != AC_SUCCESS) {
        return "Error freeing index.";
    }


    return NULL;
}

char *
test_cannot_fix_when_already_fixed(void)
{
    ac_index *index = ac_index_new();
    ac_index_fix(index);

    if (ac_index_fix(index) != AC_FAILURE) {
        return "Expected fix to fail when already fixed.";
    }

    return NULL;
}

char *
test_cannot_enter_when_already_fixed(void)
{
    ac_index *index = ac_index_new();
    ac_index_fix(index);

    char *results = NULL;
    if (ac_index_enter(index, "foo", 3, "FOO") != AC_FAILURE) {
        return "Expected enter to fail after fix.";
    }

    return NULL;
}

char *
test_cannot_query_until_fixed(void)
{
    ac_index *index = ac_index_new();
    ac_index_enter(index, "hers", 4, "HERS");

    char *phrase = "this here is history";
    //              .123456789.123456789
    //               --- --      ---
    int phrase_length = strlen(phrase);

    char *results = NULL; 

    if (ac_index_query_cb(index,
                          phrase,
                          phrase_length,
                          collect_results,
                          &results) != AC_FAILURE) {
        return "Expected query to fail without fix.";
    }

    return NULL;
}


ac_error_code
ignore_results(void *context, ac_result *result)
{
    return AC_SUCCESS;
}

ac_error_code
decref(void *item, void *data)
{
    int *refcount = (int *) item;
    (*refcount)--;
    return AC_SUCCESS;
}

char *
test_objects_for_common_endings_are_freed_correctly(void)
{
    ac_index *index = ac_index_new();

    int refcount = 0;
    ac_index_enter(index, "food", 4, &refcount);
    ++refcount;

    ac_index_enter(index, "ood", 3, &refcount);
    ++refcount;

    ac_index_fix(index);

    char *phrase = "blah";
    void *results = NULL;
    ac_index_query_cb(index,
                      phrase,
                      strlen(phrase),
                      collect_results,
                      &results);

    ac_index_free(index, decref);

    if (refcount != 0) {
        char *message = NULL;
        asprintf(&message, "Expected refcount to be 0 but was %d", refcount);
        return message;
    }

    return NULL;
}

typedef char * (*test_case)(void);

int
main(void)
{
    test_case tests[] = {
        test_init,
        test_query,
        test_cannot_fix_when_already_fixed,
        test_cannot_enter_when_already_fixed,
        test_cannot_query_until_fixed,
        test_objects_for_common_endings_are_freed_correctly,
        NULL
    };

    for (test_case *testp = tests; *testp; ++testp) {
        test_case test = *testp;
        char *result = test();
        if (result) {
            printf("\nFAILURE: %s\n", result);
            return 1;
        } else {
            printf(".");
        }
    }

    printf("\n");
    return 0;
}
