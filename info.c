/*
 * MIT License
 * 
 * Copyright (c) 2020 Sujanan Bhathiya
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

