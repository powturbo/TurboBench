
#define _FILE_OFFSET_BITS 64  // In Linux make sizeof(off_t) == 8
#include <csa_common.h>
#include <csa_typedef.h>
#include <csa_thread.h>
#include <csa_file.h>
#include <csa_worker.h>
#include <csa_io.h>
#include <csa_indexpack.h>
#include <csa_progress.h>

// Return true if strings a == b or a+"/" is a prefix of b
// or a ends in "/" and is a prefix of b.
// Match ? in a to any char in b.
// Match * in a to any string in b.
// In Windows, not case sensitive.
bool ispath(const char* a, const char* b) {
  for (; *a; ++a, ++b) {
    const int ca=tolowerW(*a);
    const int cb=tolowerW(*b);
    if (ca=='*') {
      while (true) {
        if (ispath(a+1, b)) return true;
        if (!*b) return false;
        ++b;
      }
    }
    else if (ca=='?') {
      if (*b==0) return false;
    }
    else if (ca==cb && ca=='/' && a[1]==0)
      return true;
    else if (ca!=cb)
      return false;
  }
  return *b==0 || *b=='/';
}

class CSArc {
    bool isselected(const char* filename);// by files, -only, -not
    void scandir(string filename, bool recurse=true);  // scan dirs to dt
    void addfile(string filename, int64_t edate, int64_t esize,
               int64_t eattr);          // add external file to dt
    int check_header();


    FileIndex index_;
    ABIndex abindex_;
    string arcname_;           // archive name
    vector<string> filenames_;     // filename args
    bool recurse_;
    bool verbose_;
    bool overwrite_;
    int mt_count_;
    int split_count_;
    string to_dir_;

    int level_;
    uint32_t dict_size_;
    void compress_index();
    void decompress_index();
    void compress_mt(vector<MainTask> &tasks);
    int decompress_mt(vector<MainTask> &tasks);

public:
    int Add();                
    int Extract();            
    int List();               
    int Test();               
    void Usage();             
    int ParseArg(int argc, const char *argv[]);
};

bool compareFuncByPosblock(FileBlock a, FileBlock b) {
    return a.posblock < b.posblock;
}


bool compareFuncByExt(IterFileEntry a, IterFileEntry b) {
    int ret = memcmp(a->second.ext, b->second.ext, 4);
    if (ret != 0)
        return ret < 0;
    else {
        // do not sort very small files
        if (a->second.esize > 64 * 1024 || b->second.esize > 64 * 1024)
            return a->second.esize < b->second.esize;
        else {
            return a->first < b->first;
        }
    }
}

bool compareFuncByTaskSize(MainTask a, MainTask b) {
    return a.total_size > b.total_size;
}

void CSArc::Usage()
{
    fprintf(stderr, "Usage of CSArc:\n");
    fprintf(stderr, "Create a new archive:\n");
    fprintf(stderr, "\tcsarc a [options] archive_name file2 file2 ...\n");
    fprintf(stderr, "\t      [options] can be:\n");
    fprintf(stderr, "\t         -m[1..5]   Compression level from most efficient to strongest\n");
    fprintf(stderr, "\t         -d##[k|m]  Dictionary size, must be range in [32KB, 1GB]\n");
    fprintf(stderr, "\t         -r         Recursively adding files in directories\n");
    fprintf(stderr, "\t         -f         Forcely overwrite existing archive\n");
    fprintf(stderr, "\t         -p##       Only works with single file compression, split\n");
    fprintf(stderr, "\t         -t#        Multithreading-number, range in [1,8]\n");
    fprintf(stderr, "\t                    Memory usage will be multiplied by this number\n");
    fprintf(stderr, "\t\n");
    fprintf(stderr, "Extract file(s) from archive:\n");
    fprintf(stderr, "\tcsarc x [options] archive_name [file1_in_arc file2_in_arc ...]\n");
    fprintf(stderr, "\t      [options] can be:\n");
    fprintf(stderr, "\t         -t#        Multithreading-number, range in [1,8]\n");
    fprintf(stderr, "\t                    Memory usage will be multiplied by this number\n");
    fprintf(stderr, "\t         -o out_dir Extraction output directory\n");
    fprintf(stderr, "\t\n");
    fprintf(stderr, "List file(s) in archive:\n");
    fprintf(stderr, "\tcsarc l [options] archive_name [file1_in_arc file2_in_arc ...]\n");
    fprintf(stderr, "\t      [options] can be:\n");
    fprintf(stderr, "\t         -v         Shows fragment information with Adler32 hash\n");
    fprintf(stderr, "\t\n");
    fprintf(stderr, "Test to extract file(s) in archive:\n");
    fprintf(stderr, "\tcsarc t [options] archive_name [file1_in_arc file2_in_arc ...]\n");
    fprintf(stderr, "\t      [options] can be:\n");
    fprintf(stderr, "\t         -t#        Multithreading-number, range in [1,8]\n");
    fprintf(stderr, "\t\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "\tcsarc a -m2 -d64m -r -t2 out.csa /disk2/*\n");
    fprintf(stderr, "\tcsarc x -t2 -o /tmp/ out.csa *.jpg\n");
    fprintf(stderr, "\tcsarc l out.csa\n");
    fprintf(stderr, "\tcsarc t out.csa *.dll\n");
}

int CSArc::ParseArg(int argc, const char *argv[])
{
    int i = 0;

    dict_size_ = 32000000;
    level_ = 2;
    mt_count_ = 1;
    split_count_ = 1;
    verbose_ = false;
    overwrite_ = false;
    recurse_ = false;

    to_dir_ = "./";
    for(; i < argc; i++)
        if (argv[i][0] == '-') {
            if (strncmp(argv[i], "-m", 2) == 0) {
                if (argv[i][2])
                    level_ = argv[i][2] - '0';
                else {
                    fprintf(stderr, "-m option parse error\n");
                    return -1;
                }
            } else if (strncmp(argv[i], "-d", 2) == 0) {
                int slen = strlen(argv[i]);
                dict_size_ = atoi(argv[i] + 2);
                if ((argv[i][slen - 1] | 0x20) == 'k') 
                    dict_size_ *= 1024;
                else if ((argv[i][slen - 1] | 0x20) == 'm')
                    dict_size_ *= 1024 * 1024;
                if (dict_size_ < 32 * 1024 || dict_size_ > 1024 * 1024 * 1024) {
                    fprintf(stderr, "-d option parse error, and should be between 32KB to 1GB\n");
                    return -1;
                }
            } else if (strncmp(argv[i], "-r", 2) == 0) {
                recurse_ = true;
            } else if (strncmp(argv[i], "-f", 2) == 0) {
                overwrite_ = true;
            } else if (strncmp(argv[i], "-v", 2) == 0) {
                verbose_ = true;
            } else if (strncmp(argv[i], "-t", 2) == 0) {
                if (argv[i][2])
                    mt_count_ = argv[i][2] - '0';
                else {
                    fprintf(stderr, "-t option parse error\n");
                    return -1;
                }
            } else if (strcmp(argv[i], "-o") == 0) {
                if (argv[i][2] == '\0') {
                    i++;
                    to_dir_ = argv[i];
                } else {
                    to_dir_ = argv[i] + 2;
                }
            } else if (strncmp(argv[i], "-p ", 2) == 0) {
                split_count_ = atoi(argv[i] + 2);
            } else 
                return -1;
        } else 
            break;

    if (i == argc)
        return -1;

    if (split_count_ <= 0)
        split_count_ = 1;
    mt_count_ = mt_count_ < 1? 1 : mt_count_;
    mt_count_ = mt_count_ > 8? 8 : mt_count_;

    arcname_ = argv[i];
    i++;
    for(; i < argc; i++)
        filenames_.push_back(argv[i]);

    return 0;
}

int show_progress(void *p, UInt64 insize, UInt64 outsize)
{
    (void)p;
    printf("\r%llu -> %llu\t\t\t\t", insize, outsize);
    fflush(stdout);
    return 0;
}

void CSArc::compress_index()
{
    uint64_t index_size = 0;
    char *index_buf = PackIndex(index_, abindex_, index_size);

    FileWriter file_writer;
    MemReader mem_reader;
    Mutex arc_lock;

    mem_reader.ptr = index_buf;
    mem_reader.size = index_size;
    mem_reader.pos = 0;
    mem_reader.is.Read = mem_read_proc;

    // begin to compress index, and write to end of file
    init_mutex(arc_lock);
    ArchiveBlocks ab;
    uint64_t arc_index_pos = 0;
    {
        InputFile f;
        f.open(arcname_.c_str());
        f.seek(0, SEEK_END);
        arc_index_pos = f.tell();
        f.close();

        ab.filename = arcname_;
        ab.blocks.clear();
    }

    file_writer.obj = new AsyncArchiveWriter(ab, 1 * 1048576, arc_lock);
    file_writer.os.Write = file_write_proc;
    {
        CSCProps p;
        CSCEncProps_Init(&p, 256 * 1024, 2);
        CSCEncHandle h = CSCEnc_Create(&p, (ISeqOutStream*)&file_writer, NULL);
        uint8_t buf[CSC_PROP_SIZE];
        CSCEnc_WriteProperties(&p, buf, 0);

        file_writer.obj->Run();
        file_writer.obj->Write(buf, CSC_PROP_SIZE);

        CSCEnc_Encode(h, (ISeqInStream*)&mem_reader, NULL);
        CSCEnc_Encode_Flush(h);
        CSCEnc_Destroy(h);
        file_writer.obj->Finish();
        delete file_writer.obj;
    }
    destroy_mutex(arc_lock);
    delete[] index_buf;

    // Write pos of compressed index and its size on header pos
    {
        OutputFile f;
        f.open(arcname_.c_str());
        f.seek(0, SEEK_END);
        uint32_t idx_compressed = f.tell() - arc_index_pos;
        char fs_buf[16];
        //printf("llu compressed_size %lu\n", arc_index_pos, idx_compressed);
        Put8(arc_index_pos, fs_buf);
        Put4(idx_compressed, fs_buf + 8);
        Put4(index_size, fs_buf + 12);
        f.write(fs_buf, 8, 16);
        fs_buf[0] = 'C'; fs_buf[1] = 'S'; fs_buf[2] = 'A';  fs_buf[7] = '1';
        Put4(0x20130331, fs_buf + 3);
        f.write(fs_buf, 0, 8);
        f.close();
    }
}

void CSArc::decompress_index()
{
    char buf[16];
    uint64_t index_pos;
    uint32_t compressed_size;
    uint32_t raw_size;
    InputFile f;

    f.open(arcname_.c_str());
    f.seek(8, SEEK_SET);
    f.read(buf, 16);
    char *tmp = buf;
    tmp = Get8(index_pos, tmp);
    tmp = Get4(compressed_size, tmp);
    tmp = Get4(raw_size, tmp);
    index_.clear();
    abindex_.clear();

    MemReader reader;
    reader.is.Read = mem_read_proc;
    reader.ptr = new char[compressed_size];
    reader.size = compressed_size;
    reader.pos = CSC_PROP_SIZE;
    f.seek(index_pos, SEEK_SET);
    f.read(reader.ptr, compressed_size);
    f.close();

    MemWriter writer;
    writer.os.Write = mem_write_proc;
    writer.ptr = new char[raw_size];
    writer.size = raw_size;
    writer.pos = 0;

    CSCProps p;
    CSCDec_ReadProperties(&p, (uint8_t*)reader.ptr);
    CSCDecHandle h = CSCDec_Create(&p, (ISeqInStream*)&reader, NULL);
    CSCDec_Decode(h, (ISeqOutStream*)&writer, NULL);
    CSCDec_Destroy(h);

    UnpackIndex(index_, abindex_, writer.ptr, raw_size);
    delete[] reader.ptr;
    delete[] writer.ptr;

    /*
    for(IterFileEntry it = index_.begin(); it != index_.end(); it++)
        printf("%s %llu %llu\n", it->first.c_str(), it->second.esize, it->second.edate);
    printf("==============\n");
    */
}

void CSArc::compress_mt(vector<MainTask> &tasks)
{
    CompressionWorker *workers[8];
    uint32_t workertasks[8];

    Mutex arc_lock;
    init_mutex(arc_lock);
    Semaphore sem_workers;
    sem_workers.init(mt_count_);

    for(int i = 0; i < mt_count_; i++) {
        workers[i] = new CompressionWorker(sem_workers, arc_lock, level_, dict_size_);
        workers[i]->Run();
        workertasks[i] = tasks.size();
    }

    abindex_.clear();
    std::sort(tasks.begin(), tasks.end(), compareFuncByTaskSize);
    // sem_workers has initilized with value (mt_count_)

    ProgressIndicator pi(tasks, (const MainWorker**)workers, mt_count_);
    pi.Run();

    for(uint32_t i = 0; i < tasks.size() + mt_count_; i++) {
        sem_workers.wait();
        for(int j = 0; j < mt_count_; j++) {
            // check which one is finished
            if (workers[j]->TaskDone()) {
                // update index based on info generated while compression
                uint32_t taskid = workertasks[j];
                if (taskid < tasks.size()) {
                    // means a task is finished
                    vector<FileBlock>& filelist = tasks[taskid].filelist;
                    for(size_t i = 0; i < filelist.size(); i++) {
                        FileBlock &b = filelist[i];
                        IterFileEntry it = index_.find(b.filename);
                        assert(it != index_.end());
                        FileEntry::Frag pib;
                        pib.bid = taskid;
                        pib.posblock = b.posblock;
                        pib.size = b.size;
                        pib.posfile = b.off;
                        pib.checksum = b.checksum;
                        it->second.frags.push_back(pib);
                    }
                    tasks[taskid].finished = true;
                }
                // mark it as a invalid value
                workertasks[j] = tasks.size();

                // don't add task for the last several(mt_count_) finishing thread
                if (i < tasks.size()) {
                    // keep adding remind tasks
                    // task id is always equal to archive id 
                    abindex_.insert(make_pair(i, ArchiveBlocks()));
                    abindex_[i].filename = arcname_;
                    workers[j]->PutTask(tasks[i], abindex_[i]);;
                    workertasks[j] = i;
                    break;
                }
            }
        }
    }

    destroy_mutex(arc_lock);

    for(int i = 0; i < mt_count_; i++) {
        workers[i]->Finish();
        delete workers[i];
    }
    pi.Finish();
}

int CSArc::decompress_mt(vector<MainTask> &tasks)
{
    DecompressionWorker *workers[8];
    uint32_t workertasks[8];

    Semaphore sem_workers;
    sem_workers.init(mt_count_);
    int decomp_ret = 0;

    for(int i = 0; i < mt_count_; i++) {
        workers[i] = new DecompressionWorker(sem_workers);
        workers[i]->Run();
        workertasks[i] = tasks.size();
    }

    std::sort(tasks.begin(), tasks.end(), compareFuncByTaskSize);
    for(uint32_t i = 0; i < tasks.size(); i++)
        std::sort(tasks[i].filelist.begin(), tasks[i].filelist.end(), compareFuncByPosblock);

    ProgressIndicator pi(tasks, (const MainWorker**)workers, mt_count_);
    pi.Run();

    for(uint32_t i = 0; i < tasks.size(); i++) {
        sem_workers.wait();
        for(int j = 0; j < mt_count_; j++) {
            if (workers[j]->TaskDone()) {
                uint32_t taskid = workertasks[j];
                if (taskid < tasks.size())
                    tasks[taskid].finished = true;
                abindex_[tasks[i].ab_id].filename = arcname_;
                workers[j]->PutTask(tasks[i], abindex_[tasks[i].ab_id]);;
                workertasks[j] = i;
                if (workers[j]->LastReturn() < 0)
                    decomp_ret = workers[j]->LastReturn();
                break;
            }
        }
    }

    for(int i = 0; i < mt_count_; i++) {
        sem_workers.wait();
        for(int j = 0; j < mt_count_; j++) {
            if (workers[j]->TaskDone()) {
                uint32_t taskid = workertasks[j];
                if (taskid < tasks.size())
                    tasks[taskid].finished = true;
                if (workers[j]->LastReturn() < 0)
                    decomp_ret = workers[j]->LastReturn();
            }
        }
    }

    for(int i = 0; i < mt_count_; i++) {
        workers[i]->Finish();
        delete workers[i];
    }
    pi.Finish();

    if (decomp_ret < 0) {
        fprintf(stderr, "Extraction error, archive corrupted\n");
        return -1;
    }
    return 0;
}

int CSArc::Add()
{
    {
        // check if file already exists
        InputFile f;
        f.open(arcname_.c_str());
        if (f.isopen() && !overwrite_) {
            fprintf(stderr, "Archive %s already exists, use -f to force overwrite\n", arcname_.c_str());
            return 1;
        }
        f.close();
    }


    for (size_t i = 0; i < filenames_.size(); ++i) {
        //printf("Filenames: %s\n", filenames_[i].c_str());
        scandir(filenames_[i].c_str(), recurse_);
    }

    vector<IterFileEntry> itlist;
    for(IterFileEntry it = index_.begin(); it != index_.end(); it++) {
        if (*it->first.rbegin() == '/' ) 
            continue;
        itlist.push_back(it);
        //printf("%s: %lld\n", it->first.c_str(), it->second.esize);
        size_t dot = it->first.find_last_of('.');
        size_t slash = it->first.find_last_of('/');
        memset(it->second.ext, 0, 5);
        if (dot == string::npos
            || (slash != string::npos && dot < slash)) {
            // do nothing
        } else {
            for(size_t i = 0; i < 4 && i + dot + 1 < it->first.size(); i++)
                it->second.ext[i] = tolower(it->first[i + dot + 1]);
        }
    }

    std::sort(itlist.begin(), itlist.end(), compareFuncByExt);

    vector<MainTask> tasks;

    uint32_t valid_file_count = 0;
    bool single_file = false;
    IterFileEntry sit; // single file iterator
    for(size_t i = 0; i < itlist.size(); i++) {
        if (itlist[i]->second.esize > 0)
            valid_file_count++;
        if (valid_file_count == 1) {
            single_file = true;
            sit = itlist[i];
        } else if (valid_file_count > 1) {
            single_file = false;
            break;
        }
    }

    if (single_file) {
        uint64_t split_size = sit->second.esize / split_count_;
        split_size = split_size < 1048576 ? 1048576 : split_size;
        split_size += 4;
        uint64_t off = 0;
        while(off < (uint64_t)sit->second.esize) {
            MainTask curtask;
            uint64_t bsize = std::min<uint64_t>(split_size, sit->second.esize - off);
            curtask.push_back(sit->first, off, bsize, 0, 0);
            tasks.push_back(curtask);
            off += bsize;
        }
    } else {
        MainTask curtask;
        for(size_t i = 0; i < itlist.size(); i++) {
            IterFileEntry it = itlist[i];
            // mini solid block size is 64 * KB
            if (i && strncmp(it->second.ext, itlist[i-1]->second.ext, 4) && curtask.total_size > 64 * 1024) {
                tasks.push_back(curtask);
                curtask.clear();
            }
            curtask.push_back(it->first, 0, it->second.esize, 0, 0);
        }
        if (curtask.total_size)
            tasks.push_back(curtask);
    }

    {
        OutputFile f;
        f.open(arcname_.c_str());
        f.truncate(24);
        f.close();
    }

    compress_mt(tasks);
    compress_index();

    {
        OutputFile f;
        f.open(arcname_.c_str());
        f.seek(0, SEEK_END);
        uint64_t compressed_size = f.tell();
        f.close();
        printf("Compressed Size: %llu\n", compressed_size);
    }
    return 0;
}

int CSArc::check_header()
{
    InputFile f;
    char buf[8];
    f.open(arcname_.c_str());
    if (!f.isopen()) {
        fprintf(stderr, "Can not open file %s\n", arcname_.c_str());
        return -1;
    }
    f.read(buf, 8);
    f.close();
    uint32_t num;
    Get4(num, buf + 3);
    if (num != 0x20130331 || buf[0] != 'C'
            || buf[1] != 'S' || buf[2] != 'A' || buf[7] != '1') {
        fprintf(stderr, "Invalid csarc file\n");
        return -1;
    }
    return 0;
}

int CSArc::Extract()
{
    if (check_header() < 0)
        return 1;
    decompress_index();

    vector<MainTask> tasks;
    map<uint64_t, size_t> idmap;

    for(IterFileEntry it = index_.begin(); it != index_.end(); it++) {
        //printf("%s -> blocks: %d\n", it->first.c_str(), it->second.frags.size());
        if (filenames_.size() && !isselected(it->first.c_str()))
            continue;
        string new_filename = it->first;
        if (new_filename.size() > 1 && new_filename[1] == ':') {
            if (new_filename.size() > 2 && (new_filename[2] == '/' || new_filename[2] == '\\'))
                new_filename = new_filename.substr(0, 1) + new_filename.substr(2);
            else
                new_filename[1] = '/';
        }
        if (new_filename[0] != '/' && *to_dir_.rbegin() != '/')
            new_filename = to_dir_ + '/' + new_filename;
        else
            new_filename = to_dir_ + new_filename;
        for(size_t i = 0; i < new_filename.size(); i++)
            if (new_filename[i] == '\\')
                new_filename[i] = '/';

        for(size_t fi = 0; fi < it->second.frags.size(); fi++) {
            MainTask *task = NULL;
            if (idmap.count(it->second.frags[fi].bid) == 0) {
                idmap[it->second.frags[fi].bid] = tasks.size();
                tasks.push_back(MainTask());
                task = &tasks[idmap[it->second.frags[fi].bid]];
                task->ab_id = it->second.frags[fi].bid;
            } else 
                task = &tasks[idmap[it->second.frags[fi].bid]];
            FileEntry::Frag& ff = it->second.frags[fi];
            if (ff.size)
                task->push_back(new_filename, ff.posfile, ff.size, ff.posblock, ff.checksum, it);
        }
        makepath(new_filename, it->second.edate, it->second.eattr);
        if (*new_filename.rbegin() != '/') {
            OutputFile f;
            f.open(new_filename.c_str());
            f.truncate();
            f.close(it->second.edate, it->second.eattr);
        }
    }
    if (decompress_mt(tasks) < 0)
        return -1;
    else
        return 0;
}

int CSArc::List()
{
    if (check_header() < 0)
        return -1;
    decompress_index();
    for(IterFileEntry it = index_.begin(); it != index_.end(); it++) {
        if (filenames_.size() && !isselected(it->first.c_str()))
            continue;
        if (verbose_)
        for(size_t i = 0; i < it->second.frags.size(); i++) {
            printf("Fragment %1d, in archive block %lu, Adler32: 0x%08x\t\t", 
                    i, it->second.frags[i].bid, it->second.frags[i].checksum);
            if (i + 1 < it->second.frags.size())
                printf("\n");
        }
        printf("%s %lld\t\t\t\t", it->first.c_str(), it->second.esize);
        printf("\n");
    }
    return 0;
}

int CSArc::Test()
{
    if (check_header() < 0)
        return -1;
    decompress_index();

    vector<MainTask> tasks;
    map<uint64_t, size_t> idmap;

    for(IterFileEntry it = index_.begin(); it != index_.end(); it++) {
        //printf("%s -> blocks: %d\n", it->first.c_str(), it->second.frags.size());
        if (filenames_.size() && !isselected(it->first.c_str()))
            continue;

        for(size_t fi = 0; fi < it->second.frags.size(); fi++) {
            MainTask *task = NULL;
            if (idmap.count(it->second.frags[fi].bid) == 0) {
                idmap[it->second.frags[fi].bid] = tasks.size();
                tasks.push_back(MainTask());
                task = &tasks[idmap[it->second.frags[fi].bid]];
                task->ab_id = it->second.frags[fi].bid;
            } else 
                task = &tasks[idmap[it->second.frags[fi].bid]];
            FileEntry::Frag& ff = it->second.frags[fi];
            if (ff.size)
                task->push_back(DUMMY_FILENAME, ff.posfile, ff.size, ff.posblock, ff.checksum, it);
        }
    }
    if (decompress_mt(tasks) < 0)
        return -1;
    else
        return 0;
}

// Return the part of fn up to the last slash
string path(const string& fn) {
  int n=0;
  for (int i=0; fn[i]; ++i)
    if (fn[i]=='/' || fn[i]=='\\') n=i+1;
  return fn.substr(0, n);
}

void CSArc::scandir(string filename, bool recurse) {

    /*
  // Don't scan diretories excluded by -not
  for (int i=0; i<notfiles.size(); ++i)
    if (ispath(notfiles[i].c_str(), filename.c_str()))
      return;
      */

#ifdef unix

  // Add regular files and directories
  while (filename.size()>1 && filename[filename.size()-1]=='/')
    filename=filename.substr(0, filename.size()-1);  // remove trailing /
  struct stat sb;
  if (!lstat(filename.c_str(), &sb)) {
    if (S_ISREG(sb.st_mode))
      addfile(filename, decimal_time(sb.st_mtime), sb.st_size,
              'u'+(sb.st_mode<<8));

    // Traverse directory
    if (S_ISDIR(sb.st_mode)) {
      addfile(filename=="/" ? "/" : filename+"/", decimal_time(sb.st_mtime),
              0, 'u'+(sb.st_mode<<8));
      if (recurse) {
        DIR* dirp=opendir(filename.c_str());
        if (dirp) {
          for (dirent* dp=readdir(dirp); dp; dp=readdir(dirp)) {
            if (strcmp(".", dp->d_name) && strcmp("..", dp->d_name)) {
              string s=filename;
              if (s!="/") s+="/";
              s+=dp->d_name;
              scandir(s, recurse);
            }
          }
          closedir(dirp);
        }
        else
          perror(filename.c_str());
      }
    }
  }
  else if (recurse || errno!=ENOENT)
    perror(filename.c_str());

#else  // Windows: expand wildcards in filename

  // Expand wildcards
  WIN32_FIND_DATA ffd;
  string t=filename;
  if (t.size()>0 && t[t.size()-1]=='/') {
    if (recurse) t+="*";
    else filename=t=t.substr(0, t.size()-1);
  }
  HANDLE h=FindFirstFile(utow(t.c_str(), true).c_str(), &ffd);
  if (h==INVALID_HANDLE_VALUE && (recurse ||
      (GetLastError()!=ERROR_FILE_NOT_FOUND &&
       GetLastError()!=ERROR_PATH_NOT_FOUND)))
    winError(t.c_str());
  while (h!=INVALID_HANDLE_VALUE) {

    // For each file, get name, date, size, attributes
    SYSTEMTIME st;
    int64_t edate=0;
    if (FileTimeToSystemTime(&ffd.ftLastWriteTime, &st))
      edate=st.wYear*10000000000LL+st.wMonth*100000000LL+st.wDay*1000000
            +st.wHour*10000+st.wMinute*100+st.wSecond;
    const int64_t esize=ffd.nFileSizeLow+(int64_t(ffd.nFileSizeHigh)<<32);
    const int64_t eattr='w'+(int64_t(ffd.dwFileAttributes)<<8);

    // Ignore links, the names "." and ".." or any unselected file
    t=wtou(ffd.cFileName);
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT
        || t=="." || t=="..") edate=0;  // don't add
    string fn=path(filename)+t;

    // Save directory names with a trailing / and scan their contents
    // Otherwise, save plain files
    if (edate) {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) fn+="/";
      addfile(fn, edate, esize, eattr);
      if (recurse && (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        fn+="*";
        scandir(fn, recurse);
      }
    }
    if (!FindNextFile(h, &ffd)) {
      if (GetLastError()!=ERROR_NO_MORE_FILES) winError(fn.c_str());
      break;
    }
  }
  FindClose(h);
#endif
}

void CSArc::addfile(string filename, int64_t edate,
                    int64_t esize, int64_t eattr) {
  if (!isselected(filename.c_str())) return;
  FileEntry& fn = index_[filename];
  fn.edate=edate;
  fn.esize=esize;
  fn.eattr= eattr; //noattributes?0:eattr;
  //fn.written=0;
}

bool CSArc::isselected(const char* filename) {
  bool matched=true;
  if (filenames_.size()>0) {
    matched=false;
    for (size_t i=0; i < filenames_.size() && !matched; ++i)
      if (ispath(filenames_[i].c_str(), filename))
        matched=true;
  }
  return matched;
}

#ifdef unix
int main(int argc, const char** argv) {
#else
int main() {
    int argc=0;
    LPWSTR* argw=CommandLineToArgvW(GetCommandLine(), &argc);
    vector<string> args(argc);
    vector<const char *> argp(argc);
    for (int i=0; i<argc; ++i) {
        args[i]=wtou(argw[i]);
        argp[i]=args[i].c_str();
    }
    const char** argv=&argp[0];
#endif
    CSArc csarc;
    fprintf(stderr, "CSArc 3.3, experimental archiver by Siyuan Fu\n (https://github.com/fusiyuan2010)\n");

    if (argc < 3) {
        fprintf(stderr, "At least two arguments, command and archive name\n");
        csarc.Usage();
        return 1;
    }

    char op = argv[1][0];

    if (csarc.ParseArg(argc - 2, argv + 2) < 0) {
        csarc.Usage();
        return 1;
    }

    int ret = 0;
    switch(op) {
        case 'a':
            ret = csarc.Add();
            break;
        case 't':
            ret = csarc.Test();
            break;
        case 'l':
            ret = csarc.List();
            break;
        case 'x':
            ret = csarc.Extract();
            break;
        default:
            csarc.Usage();
            fprintf(stderr, "Invalid command '%c'\n", op);
            return 1;
    }

    return ret;
}



