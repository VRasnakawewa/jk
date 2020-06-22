#include "info.h"

int infoVerify(struct map *info)
{
    return 1;
}

int infoGetFileMode(struct map *info)
{
    return mapHas(info, "files") ? INFO_MULI_FILE : INFO_SINGLE_FILE;
}

i64 infoGetTotalBytes(struct map *info)
{
    /* single file mode */
    if (infoGetFileMode(info) == INFO_SINGLE_FILE)
        return benAsI64(mapGet(info, "length"));

    /* multi file mode */
    struct list *files = benAsList(mapGet(info, "files"));

    u64 length = 0;
    for (u64 i = 0; i < files->len; i++) {
        length += benAsI64(
            mapGet(benAsMap(files->values[i]), "length"));
    }
    return length;
}

i64 infoGetPieceLen(struct map *info)
{
    return benAsI64(mapGet(info, "piece length"));
}

void infoFileIterInit(struct infoFileIter *it, struct map *info)
{
    it->pos = 0;
    it->mode = infoGetFileMode(info);
    it->length = 0;
    it->files = NULL;
    it->name = benAsJstr(mapGet(info, "name"));
    if (it->mode == INFO_SINGLE_FILE)
        it->length = benAsI64(mapGet(info, "length"));
    else
        it->files = benAsList(mapGet(info, "files"));
}

int infoFileIterNext(struct infoFileIter *it, struct infoFile *file)
{
    if (it->mode == INFO_SINGLE_FILE) {
        file->length = it->length;
        file->name = it->name;
        file->path = NULL;
        return 0;
    }
    struct map *cur = benAsMap(it->files->values[it->pos++]);
    file->name =  it->name;
    file->length = benAsI64(mapGet(cur, "length"));
    file->path = benAsJstr(mapGet(cur, "path"));
    return (it->pos >= it->files->len) ? 0 : 1;
}

