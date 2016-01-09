#include <csa_indexpack.h>
#include <string.h>
using std::make_pair;

char *Put8(uint64_t i, char *buf)
{
    for(int j = 0; j < 8; j++) {
        *buf++ = (i & 0xFF);
        i >>= 8;
    }
    return buf;
}

char *Get8(uint64_t& i, char *buf)
{
    i = 0;
    for(int j = 7; j >= 0; j--) {
        i = (i << 8) + (uint8_t)buf[j];
    }
    return buf + 8;
}

char *Get8(int64_t& i, char *buf)
{
    uint64_t j;
    char *ret = Get8(j, buf);
    i = j;
    return ret;
}

char *Put4(uint32_t i, char *buf)
{
    for(int j = 0; j < 4; j++) {
        *buf++ = (i & 0xFF);
        i >>= 8;
    }
    return buf;
}

char *Get4(uint32_t& i, char *buf)
{
    i = 0;
    for(int j = 3; j >= 0; j--) {
        i = (i << 8) + (uint8_t)buf[j];
    }
    return buf + 4;
}

char *Put2(uint16_t i, char *buf)
{
    *buf++ = (i & 0xFF);
    *buf++ = (i >> 8);
    return buf;
}

char *Get2(uint64_t& i, char *buf)
{
    i = 0;
    for(int j = 1; j >= 0; j--) {
        i = (i << 8) + (uint8_t)buf[j];
    }
    return buf + 2;
}



static uint64_t FileEntrySize(IterFileEntry it)
{
    return 4 + it->first.size()
        // file name len
        + 3 * 8 + 1 
        // data size eattr, frags num
        + it->second.frags.size() * (4 + 4 + 24);
        // frags info
}

static char *FileEntryToBuf(IterFileEntry it, char *buf)
{
    buf = Put4(it->first.size(), buf);
    memcpy(buf, it->first.c_str(), it->first.size());
    buf += it->first.size();
    buf = Put8(it->second.edate, buf);
    buf = Put8(it->second.esize, buf);
    buf = Put8(it->second.eattr, buf);
    *buf++ = it->second.frags.size();
    for(size_t i = 0; i < it->second.frags.size(); i++) {
        buf = Put4(it->second.frags[i].bid, buf);
        buf = Put4(it->second.frags[i].checksum, buf);
        buf = Put8(it->second.frags[i].posblock, buf);
        buf = Put8(it->second.frags[i].size, buf);
        buf = Put8(it->second.frags[i].posfile, buf);
    }
    return buf;
}

static char *BufToFileEntry(string& filename, FileEntry& fe, char *buf)
{
    uint32_t u4;
    buf = Get4(u4, buf);
    filename.assign(buf, u4);
    buf += u4;
    buf = Get8(fe.edate, buf);
    buf = Get8(fe.esize, buf);
    buf = Get8(fe.eattr, buf);
    int8_t i1 = *buf++;
    fe.frags.clear();
    for(int i = 0; i < i1; i++) {
        FileEntry::Frag f;
        buf = Get4(f.bid, buf);
        buf = Get4(f.checksum, buf);
        buf = Get8(f.posblock, buf);
        buf = Get8(f.size, buf);
        buf = Get8(f.posfile, buf);
        fe.frags.push_back(f);
    }
    return buf;
}

static uint64_t ArchiveBlocksSize(IterAB it) 
{
    return 8 + 4 + it->second.filename.size()
        // file name len
        + 4 + it->second.blocks.size() * 16;
        // blocks
}

static char *ArchiveBlocksToBuf(IterAB it, char *buf)
{
    buf = Put8(it->first, buf);
    ArchiveBlocks &abs = it->second;
    //buf = Put4(abs.filename.size(), buf);
    //memcpy(buf, abs.filename.c_str(), abs.filename.size());
    //buf += abs.filename.size();
    buf = Put4(abs.blocks.size(), buf);
    for(size_t i = 0; i < abs.blocks.size(); i++) {
        buf = Put8(abs.blocks[i].off, buf);
        buf = Put8(abs.blocks[i].size, buf);
    }
    return buf;
}

static char *BufToArchiveBlocks(uint64_t& id, ArchiveBlocks& abs, char *buf)
{
    buf = Get8(id, buf);
    uint32_t u4;
    //buf = Get4(u4, buf);
    //abs.filename.assign(buf, u4);
    //buf += u4;
    buf = Get4(u4, buf);
    abs.blocks.clear();
    for(uint32_t i = 0; i < u4; i++) {
        ArchiveBlocks::Block b;
        buf = Get8(b.off, buf);
        buf = Get8(b.size, buf);
        abs.blocks.push_back(b);
    }
    return buf;
}

char *PackIndex(FileIndex &fi, ABIndex &abi, uint64_t &total_size)
{
    total_size = 0;

    total_size += 4;
    for(IterFileEntry it = fi.begin(); it != fi.end(); it++)
        total_size += FileEntrySize(it);

    total_size += 4;
    for(IterAB it = abi.begin(); it != abi.end(); it++)
        total_size += ArchiveBlocksSize(it);

    char *buf = new char[total_size];
    char *tmp = buf;
    tmp = Put4(fi.size(), tmp);
    for(IterFileEntry it = fi.begin(); it != fi.end(); it++)
        tmp = FileEntryToBuf(it, tmp);

    tmp = Put4(abi.size(), tmp);
    for(IterAB it = abi.begin(); it != abi.end(); it++)
        tmp = ArchiveBlocksToBuf(it, tmp);
    return buf;
}

bool UnpackIndex(FileIndex &fi, ABIndex &abi, char *buf, uint64_t total_size)
{
    char *tmp = buf, *limit = buf + total_size;
    (void)total_size;

    (void)limit;
    fi.clear();
    abi.clear();
    uint32_t isize;
    tmp = Get4(isize, tmp);
    for(uint32_t i = 0; i < isize; i++) {
        string filename;
        FileEntry fe;
        tmp = BufToFileEntry(filename, fe, tmp);
        fi.insert(make_pair(filename, fe));
    }

    tmp = Get4(isize, tmp);
    for(uint32_t i = 0; i < isize; i++) {
        uint64_t id;
        ArchiveBlocks abs;
        tmp = BufToArchiveBlocks(id, abs, tmp);
        abi.insert(make_pair(id, abs));
    }
    return true;
}


