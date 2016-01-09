#include <csa_worker.h>
#include <csa_file.h>
#include <csc_enc.h>
#include <csc_dec.h>

struct ProgressObject {
    ICompressProgress p;
    MainWorker *w;
};

static int decompress_update_progress(void *p, UInt64 insize, UInt64 outsize) {
    ProgressObject *po = (ProgressObject *)p;
    po->w->processed_raw_ = outsize;
    return 0;
}

static int compress_update_progress(void *p, UInt64 insize, UInt64 outsize) {
    ProgressObject *po = (ProgressObject *)p;
    po->w->processed_raw_ = insize;
    return 0;
}

int CompressionWorker::do_work() 
{
    FileReader file_reader;
    file_reader.obj = new AsyncFileReader(task_->filelist, 
            std::min<uint64_t>(32 * 1048576, task_->total_size));
    file_reader.is.Read = file_read_proc;

    FileWriter file_writer;
    file_writer.obj = new AsyncArchiveWriter(*abs_, 8 * 1048576, arc_lock_);
    file_writer.os.Write = file_write_proc;

    CSCProps p;
    CSCEncProps_Init(&p, std::min<uint64_t>(dict_size_, task_->total_size), level_);
    CSCEncHandle h = CSCEnc_Create(&p, (ISeqOutStream*)&file_writer, NULL);
    uint8_t buf[CSC_PROP_SIZE];
    CSCEnc_WriteProperties(&p, buf, 0);

    file_reader.obj->Run();
    file_writer.obj->Run();
    file_writer.obj->Write(buf, CSC_PROP_SIZE);

    ProgressObject prog;
    prog.p.Progress = compress_update_progress;
    prog.w = this;

    int ret = CSCEnc_Encode(h, (ISeqInStream*)&file_reader, (ICompressProgress*)&prog);
    CSCEnc_Encode_Flush(h);
    CSCEnc_Destroy(h);
    file_reader.obj->Finish();
    file_writer.obj->Finish();
    delete file_writer.obj;
    delete file_reader.obj;
    return ret;
}


int DecompressionWorker::do_work() 
{
    FileReader file_reader;
    file_reader.obj = new AsyncArchiveReader(*abs_, 8 * 1048576);
    file_reader.is.Read = file_read_proc;

    FileWriter file_writer;
    file_writer.obj = new AsyncFileWriter(task_->filelist, 32 * 1048576);
    file_writer.os.Write = file_write_proc;

    file_reader.obj->Run();
    file_writer.obj->Run();

    ProgressObject prog;
    prog.p.Progress = decompress_update_progress;
    prog.w = this;

    CSCProps p;
    uint8_t buf[CSC_PROP_SIZE];
    size_t prop_size = CSC_PROP_SIZE; 
    file_reader.obj->Read(buf, &prop_size);
    CSCDec_ReadProperties(&p, buf);
    CSCDecHandle h = CSCDec_Create(&p, (ISeqInStream*)&file_reader, NULL);
    int ret = CSCDec_Decode(h, (ISeqOutStream*)&file_writer, (ICompressProgress*)&prog);
    CSCDec_Destroy(h);

    file_reader.obj->Finish();
    file_writer.obj->Finish();
    delete file_reader.obj;
    delete file_writer.obj;
    return ret;
}

