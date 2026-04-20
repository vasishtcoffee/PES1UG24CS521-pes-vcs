// commit.c — minimal commit implementation

#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// forward declarations
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);
int tree_from_index(const Index *index, ObjectID *tree_id);

// ─── COMMIT ────────────────────────────────────────────────────────────────

int commit_create(const char *message) {
    Index index;

    // load index
    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        return -1;
    }

    if (index.count == 0) {
        fprintf(stderr, "nothing to commit\n");
        return -1;
    }

    // build tree
    ObjectID tree_id;
    if (tree_from_index(&index, &tree_id) != 0) {
        fprintf(stderr, "error: failed to create tree\n");
        return -1;
    }

    // convert tree hash to hex
    char tree_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&tree_id, tree_hex);

    // build commit content
    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer),
        "tree %s\n"
        "message %s\n",
        tree_hex,
        message
    );

    // write commit object
    ObjectID commit_id;
    if (object_write(OBJ_COMMIT, buffer, len, &commit_id) != 0) {
        fprintf(stderr, "error: failed to write commit\n");
        return -1;
    }

    // update HEAD
    char commit_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&commit_id, commit_hex);

    FILE *f = fopen(".pes/HEAD", "w");
    if (!f) {
        fprintf(stderr, "error: failed to update HEAD\n");
        return -1;
    }

    fprintf(f, "%s\n", commit_hex);
    fclose(f);

    printf("Committed: %s\n", commit_hex);

    return 0;
}
