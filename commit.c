#include "commit.h"
#include "index.h"
#include "tree.h"
#include "pes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>

// forward declarations (from object.c)
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);
int object_read(const ObjectID *id, ObjectType *type_out, void **data_out, size_t *len_out);

// ─── CREATE COMMIT ─────────────────────────────────────────────

int commit_create(const char *message, ObjectID *commit_id_out) {
    if (!message || !commit_id_out) return -1;

    Commit commit;
    memset(&commit, 0, sizeof(Commit));

    // 1. Load index
    Index index;
    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        return -1;
    }

    if (index.count == 0) {
        fprintf(stderr, "nothing to commit\n");
        return -1;
    }

    // 2. Build tree from index (FIXED)
    if (tree_from_index(&index, &commit.tree) != 0) {
        fprintf(stderr, "error: failed to create tree\n");
        return -1;
    }

    // 3. Parent commit (if exists)
    if (head_read(&commit.parent) == 0) {
        commit.has_parent = 1;
    } else {
        commit.has_parent = 0;
    }

    // 4. Author
    const char *author = pes_author();
    if (!author) author = "unknown";

    snprintf(commit.author, sizeof(commit.author), "%s", author);

    // 5. Timestamp
    commit.timestamp = (uint64_t)time(NULL);

    // 6. Message
    snprintf(commit.message, sizeof(commit.message), "%s", message);

    // 7. Serialize
    void *buffer = NULL;
    size_t len = 0;

    if (commit_serialize(&commit, &buffer, &len) != 0) {
        fprintf(stderr, "error: serialize failed\n");
        return -1;
    }

    // 8. Write commit object
    if (object_write(OBJ_COMMIT, buffer, len, commit_id_out) != 0) {
        free(buffer);
        fprintf(stderr, "error: object write failed\n");
        return -1;
    }

    free(buffer);

    // 9. Update HEAD
    if (head_update(commit_id_out) != 0) {
        fprintf(stderr, "error: failed to update HEAD\n");
        return -1;
    }

    // 10. Print commit hash
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(commit_id_out, hex);

    printf("Committed: %s\n", hex);

    return 0;
}
