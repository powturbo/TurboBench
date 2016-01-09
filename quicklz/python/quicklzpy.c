/**
    This is a python module to access to compression and decompression
    in the QuickLZ library. This module only provides a few entry points from
    Python:
    
    The main functions:
        qlz_compress()
        qlz_decompress()
        qlz_size_decompressed()
        qlz_size_compressed()
    
    The state object:
        QLZStateCompress
        

  **/
#include <Python.h>
#include "quicklz.h"
#if QLZ_STREAMING_BUFFER == 0
    #error Define QLZ_STREAMING_BUFFER to a non-zero value for this module
#endif

PyTypeObject qlz_state_compress_Type;
PyTypeObject qlz_state_decompress_Type;

typedef struct {
    PyObject_HEAD
    qlz_state_compress* value;
} qlz_state_compress_;

typedef struct {
    PyObject_HEAD
    qlz_state_decompress* value;
} qlz_state_decompress_;

PyObject *
qlz_state_compress_NEW(void)
{
    qlz_state_compress_ *object = PyObject_NEW(qlz_state_compress_, &qlz_state_compress_Type);
    if (object != NULL) {
      object->value = (qlz_state_compress *)malloc(sizeof(qlz_state_compress));
      memset(object->value, 0, sizeof(qlz_state_compress));
    }
    return (PyObject *)object;
}

void
qlz_state_compress_dealloc(PyObject *self)
{
    free(((qlz_state_compress_*)self)->value);
    PyMem_DEL(self);
}

/* This is the Python constructor */
PyObject *
qlz_state_compress_new(PyObject * self, PyObject * args)
{
    return qlz_state_compress_NEW();
}

PyObject *qlz_py_getattr(PyObject *self, char * attrname)
{
    PyErr_SetString(PyExc_AttributeError, attrname);
    return NULL;
}

PyObject *
qlz_state_decompress_NEW(void)
{
    qlz_state_decompress_ *object = PyObject_NEW(qlz_state_decompress_, &qlz_state_decompress_Type);
    if (object != NULL) {
      object->value = (qlz_state_decompress *)malloc(sizeof(qlz_state_decompress));
      memset(object->value, 0, sizeof(qlz_state_decompress));
    }
    return (PyObject *)object;
}

void
qlz_state_decompress_dealloc(PyObject *self)
{
    free(((qlz_state_decompress_*)self)->value);
    PyMem_DEL(self);
}

/* This is the Python constructor */
PyObject *
qlz_state_decompress_new(PyObject * self, PyObject * args)
{
    return qlz_state_decompress_NEW();
}

PyObject *qlz_getattr(PyObject *self, char * attrname)
{
    PyErr_SetString(PyExc_AttributeError, attrname);
    return NULL;
}

int qlz_py_setattr(PyObject *self,  char *attrname, PyObject *value)
{
    PyErr_SetString(PyExc_AttributeError, attrname);
    return -1;
}

int qlz_compare(PyObject *self, PyObject *other)
{
    // 'is a' comparison
    return self == other;
}

int qlz_c_print(PyObject *self, FILE *fp, int flags)
{
    fprintf(fp, "QLZStateCompress(%ld)", (long)self);
    return 0;
}

PyObject *qlz_c_str(PyObject *self)
{
    char buf[100];
    sprintf(buf, "QLZStateCompress(%ld)", (long)self);
    return PyString_FromString(buf);
}

int qlz_d_print(PyObject *self, FILE *fp, int flags)
{
    fprintf(fp, "QLZStateDecompress(%ld)", (long)self);
    return 0;
}

PyObject *qlz_d_str(PyObject *self)
{
    char buf[100];
    sprintf(buf, "QLZStateDecompress(%ld)", (long)self);
    return PyString_FromString(buf);
}

long
qlz_hash(PyObject *self)
{
    return (long)self;
}

/**
  * The qlz_state_compress python object
  */
PyTypeObject qlz_state_compress_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "QLZStateCompress",         /* char *tp_name; */
  sizeof(qlz_state_compress_),/* int tp_basicsize; */
  0,                          /* int tp_itemsize;       /* not used much */
  qlz_state_compress_dealloc, /* destructor tp_dealloc; */
  qlz_c_print,                /* printfunc  tp_print;   */
  qlz_py_getattr,             /* getattrfunc  tp_getattr; /* __getattr__ */
  qlz_py_setattr,             /* setattrfunc  tp_setattr;  /* __setattr__ */
  qlz_compare,                /* cmpfunc  tp_compare;  /* __cmp__ */
  qlz_c_str,                  /* reprfunc  tp_repr;    /* __repr__ */
  0,                          /* PyNumberMethods *tp_as_number; */
  0,                          /* PySequenceMethods *tp_as_sequence; */
  0,                          /* PyMappingMethods *tp_as_mapping; */
  qlz_hash,                   /* hashfunc tp_hash;     /* __hash__ */
  0,                          /* ternaryfunc tp_call;  /* __call__ */
  qlz_c_str,                  /* reprfunc tp_str;      /* __str__ */
};

/**
  * The qlz_state_compress python object
  */
PyTypeObject qlz_state_decompress_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "QLZStateDecompress",       /* char *tp_name; */
  sizeof(qlz_state_decompress_),/* int tp_basicsize; */
  0,                          /* int tp_itemsize;       /* not used much */
  qlz_state_decompress_dealloc,/* destructor tp_dealloc; */
  qlz_d_print,                /* printfunc  tp_print;   */
  qlz_py_getattr,             /* getattrfunc  tp_getattr; /* __getattr__ */
  qlz_py_setattr,             /* setattrfunc  tp_setattr;  /* __setattr__ */
  qlz_compare,                /* cmpfunc  tp_compare;  /* __cmp__ */
  qlz_d_str,                  /* reprfunc  tp_repr;    /* __repr__ */
  0,                          /* PyNumberMethods *tp_as_number; */
  0,                          /* PySequenceMethods *tp_as_sequence; */
  0,                          /* PyMappingMethods *tp_as_mapping; */
  qlz_hash,                   /* hashfunc tp_hash;     /* __hash__ */
  0,                          /* ternaryfunc tp_call;  /* __call__ */
  qlz_d_str,                  /* reprfunc tp_str;      /* __str__ */
};


PyObject *qlz_size_decompressed_py(PyObject *self, PyObject *args)
{ 
    PyObject *result = NULL;
    char* buffer;
    int buffer_length;
    
    if (PyArg_ParseTuple(args, "s#", &buffer, &buffer_length)) {
        result = Py_BuildValue("i", qlz_size_decompressed(buffer));
    } /* otherwise there is an error,
       * the exception already raised by PyArg_ParseTuple, and NULL is
       * returned.
       */
    return result;
}

PyObject *qlz_size_compressed_py(PyObject *self, PyObject *args)
{ 
    PyObject *result = NULL;
    char* buffer;
    int buffer_length;
    
    if (PyArg_ParseTuple(args, "s#", &buffer, &buffer_length)) {
        result = Py_BuildValue("i", qlz_size_compressed(buffer));
    } /* otherwise there is an error,
       * the exception already raised by PyArg_ParseTuple, and NULL is
       * returned.
       */
    return result;
}

PyObject *qlz_compress_py(PyObject *self, PyObject *args)
{ 
    PyObject *result = NULL;
    PyObject *state = NULL;
    char* buffer;
    char* compressed_buffer;
    int buffer_length;
    int size_compressed;

    if (PyArg_ParseTuple(args, "s#O!",
                         &buffer, &buffer_length,
                         &qlz_state_compress_Type, &state))
    {
        compressed_buffer = (char*)malloc(buffer_length+400);
        size_compressed = qlz_compress(buffer, compressed_buffer,
                                       buffer_length,
                                       ((qlz_state_compress_*)state)->value);
        result = Py_BuildValue("s#", compressed_buffer, size_compressed);
        free(compressed_buffer);
    } /* otherwise there is an error,
       * the exception already raised by PyArg_ParseTuple, and NULL is
       * returned.
       */
    return result;
}

PyObject *qlz_decompress_py(PyObject *self, PyObject *args)
{ 
    PyObject *result = NULL;
    PyObject *state = NULL;
    char* buffer;
    char* decompressed_buffer;
    int buffer_length;
    int size_decompressed;

    if (PyArg_ParseTuple(args, "s#O!",
                        &buffer, &buffer_length,
                        &qlz_state_decompress_Type, &state))
    {
        size_decompressed = qlz_size_decompressed(buffer);
        decompressed_buffer = (char*)malloc(size_decompressed);

        qlz_decompress(buffer, decompressed_buffer,
                       ((qlz_state_decompress_*)state)->value);
        
        result = Py_BuildValue("s#", decompressed_buffer, size_decompressed);
        free(decompressed_buffer);
    } /* otherwise there is an error,
       * the exception already raised by PyArg_ParseTuple, and NULL is
       * returned.
       */
    return result;
}


PyMethodDef methods[] = {
  {"QLZStateCompress", qlz_state_compress_new, METH_VARARGS,
    "An internal object for tracking streaming compression.\n"},

  {"QLZStateDecompress", qlz_state_decompress_new, METH_VARARGS,
    "An internal object for tracking streaming decompression.\n"},

  {"qlz_size_decompressed", qlz_size_decompressed_py, METH_VARARGS,
    "qlz_size_decompressed(compressed_data)\n"
    "\n"
    "How many bytes would the uncompressed data in this chunk be?\n"},
  {"qlz_size_compressed", qlz_size_compressed_py, METH_VARARGS,
    "qlz_size_compressed(compressed_data)\n"
    "\n"
    "How many bytes is the compressed data in this chunk?\n"},

  {"qlz_compress", qlz_compress_py, METH_VARARGS,
    "qlz_compress(raw_data, state)\n"
    "Compress a chunk of data using QuickLZ.\n"
    "\n"
    "If the same state object is used to compress data sequentially,\n"
    "then chunks must also be decompressed using a state object in the same\n"
    "sequence.\n"
    "\n"
    "@param raw_data: string-like\n"
    "@param state: a QLZStateCompress object.\n"},

  {"qlz_decompress", qlz_decompress_py, METH_VARARGS,
    "qlz_decompress(compressed_chunk, state)\n"
    "Decompress a chunk of data using QuickLZ."
    "\n"
    "If the same state object is used to compress data sequentially,\n"
    "then chunks must also be decompressed using a state object in the same\n"
    "sequence.\n"
    "\n"
    "@param compressed_chunk: string-like\n"
    "@param state: a QLZStateDecompress object.\n"},

  {NULL, NULL},
};

void initquicklz(void)
{
    (void)Py_InitModule("quicklz", methods);
}

